// Copyright (c) 2024-2025 Beijing Institute of Open Source Chip (BOSC)
// Copyright (c) 2020-2025 Institute of Computing Technology, Chinese Academy of Sciences
// Copyright (c) 2020-2021 Peng Cheng Laboratory
//
// XiangShan is licensed under Mulan PSL v2.
// You can use this software according to the terms and conditions of the Mulan PSL v2.
// You may obtain a copy of Mulan PSL v2 at:
//          https://license.coscl.org.cn/MulanPSL2
//
// THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
// EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
// MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
//
// See the Mulan PSL v2 for more details.

package xiangshan.frontend.bpu.tage

import chisel3._
import chisel3.util._
import org.chipsalliance.cde.config.Parameters
import utility.XSPerfAccumulate
import utility.sram.FoldedSRAMTemplate
import utility.sram.SRAMTemplate
import xiangshan.frontend.bpu.TageTableInfo
import xiangshan.frontend.bpu.WriteBuffer

class TageMutationVersionTable(
    tableIdx:          Int,
    implicit val info: TageTableInfo
)(implicit p: Parameters) extends TageModule {
  private val VersionWidth = 12
  private val NumBuckets   = 16

  class VersionTableIO extends TageBundle {
    val predictSetIdx:     UInt                 = Input(UInt(SetIdxWidth.W))
    val predictBankIdx:    UInt                 = Input(UInt(log2Ceil(NumBanks).W))
    val predictVersion:    Vec[UInt]            = Output(Vec(NumWays, UInt(VersionWidth.W)))
    val trainSetIdx:       UInt                 = Input(UInt(SetIdxWidth.W))
    val trainBankIdx:      UInt                 = Input(UInt(log2Ceil(NumBanks).W))
    val trainVersion:      Vec[UInt]            = Output(Vec(NumWays, UInt(VersionWidth.W)))
    val staleCheckSetIdx:  UInt                 = Input(UInt(SetIdxWidth.W))
    val staleCheckBankIdx: UInt                 = Input(UInt(log2Ceil(NumBanks).W))
    val staleCheckVersion: Vec[UInt]            = Output(Vec(NumWays, UInt(VersionWidth.W)))
    val writeReq:          Valid[TableWriteReq] = Flipped(Valid(new TableWriteReq))
    val resetAll:          Bool                 = Input(Bool())
  }
  val io: VersionTableIO = IO(new VersionTableIO)

  private val versions = RegInit(VecInit.fill(NumBanks)(
    VecInit.fill(NumWays)(VecInit.fill(NumBuckets)(0.U(VersionWidth.W)))
  ))

  private def versionIndex(setIdx: UInt): UInt = {
    val hashWidth = log2Ceil(NumBuckets)
    setIdx(hashWidth - 1, 0) ^ setIdx(2 * hashWidth - 1, hashWidth)
  }

  for (wayIdx <- 0 until NumWays) {
    io.predictVersion(wayIdx) :=
      versions(io.predictBankIdx)(wayIdx)(versionIndex(io.predictSetIdx))
    io.trainVersion(wayIdx) :=
      versions(io.trainBankIdx)(wayIdx)(versionIndex(io.trainSetIdx))
    io.staleCheckVersion(wayIdx) :=
      versions(io.staleCheckBankIdx)(wayIdx)(versionIndex(io.staleCheckSetIdx))
  }

  when(io.resetAll) {
    for {
      bankIdx <- 0 until NumBanks
      wayIdx  <- 0 until NumWays
      bucket  <- 0 until NumBuckets
    } {
      versions(bankIdx)(wayIdx)(bucket) := versions(bankIdx)(wayIdx)(bucket) + 1.U
    }
  }.elsewhen(io.writeReq.valid) {
    val writeBankIdx    = OHToUInt(io.writeReq.bits.bankMask)
    val writeVersionIdx = versionIndex(io.writeReq.bits.setIdx)
    for (wayIdx <- 0 until NumWays) {
      when(io.writeReq.bits.wayMask(wayIdx)) {
        versions(writeBankIdx)(wayIdx)(writeVersionIdx) :=
          versions(writeBankIdx)(wayIdx)(writeVersionIdx) + 1.U
      }
    }
  }
}

class TageTable(
    tableIdx:          Int,
    implicit val info: TageTableInfo // declare info as implicit val to pass it to Bundles / methods like TableReadReq
)(implicit p: Parameters) extends TageModule with TableHelper {
  class TageTableIO extends TageBundle {
    val readReq:  Vec[Valid[TableReadReq]] = Flipped(Vec(2, Valid(new TableReadReq)))
    val readResp: Vec[TableReadResp]       = Output(Vec(2, new TableReadResp))

    val writeReq: Valid[TableWriteReq] = Flipped(Valid(new TableWriteReq))

    val usefulResetStart:    Bool = Input(Bool())
    val usefulResetInFlight: Bool = Output(Bool())

    val sramResetDone: Bool = Output(Bool())
  }

  val io: TageTableIO = IO(new TageTableIO)

  println(f"TageTable[$tableIdx]:")
  println(f"  Size(set, bank, way): $NumSets * $NumBanks * $NumWays = ${info.Size}")
  println(f"  History length: ${info.HistoryLength}")
  println(f"  Address fields:")
  addrFields.show(indent = 4)

  private val entrySram =
    Seq.tabulate(NumBanks, NumWays) { (bankIdx, wayIdx) =>
      Module(new SRAMTemplate(
        new TageEntry,
        set = NumSets,
        way = 1,
        singlePort = true,
        shouldReset = true,
        withClockGate = true,
        hasMbist = hasMbist,
        hasSramCtl = hasSramCtl,
        suffix = Option("bpu_tage_entry")
      )).suggestName(s"tage_entry_sram_bank${bankIdx}_way${wayIdx}")
    }

  // Folding multiple ways of an SRAM with too small a data width together results in better area efficiency.
  private val usefulCtrSram =
    Seq.tabulate(NumBanks, NumWays) { (bankIdx, wayIdx) =>
      Module(new FoldedSRAMTemplate(
        UsefulCounter(),
        set = NumSets,
        width = NumUsefulCtrSramFolds,
        way = 1,
        singlePort = true,
        shouldReset = true,
        withClockGate = true,
        hasMbist = hasMbist,
        hasSramCtl = hasSramCtl,
        suffix = Option("bpu_tage_useful")
      )).suggestName(s"tage_useful_sram_bank${bankIdx}_way${wayIdx}")
    }

  private val usefulResetSetIdx       = RegInit(VecInit.fill(NumBanks)(0.U(SetIdxWidth.W)))
  private val usefulResetInFlightMask = RegInit(VecInit.fill(NumBanks)(false.B))

  io.usefulResetInFlight := usefulResetInFlightMask.reduce(_ || _)

  private val readDuringUsefulReset = io.readReq.map { readReq =>
    (0 until NumBanks).map { bankIdx =>
      readReq.valid && readReq.bits.bankMask(bankIdx) && usefulResetInFlightMask(bankIdx)
    }.reduce(_ || _)
  }

  // use a write buffer to store a entrySram write request
  private val entryWriteBuffers =
    Seq.tabulate(NumBanks) { bankIdx =>
      Module(new WriteBuffer(
        new EntrySramWriteReq,
        WriteBufferSize,
        numPorts = NumWays,
        hasCnt = true,
        hasLookup = true,
        nameSuffix = s"tageTable${tableIdx}_${bankIdx}"
      )).suggestName(s"tage_entry_write_buffer_bank${bankIdx}")
    }

  // use a write buffer to store a usefulCtr write request
  private val usefulCtrWriteBuffers =
    Seq.tabulate(NumBanks, NumWays) { (bankIdx, wayIdx) =>
      Module(new WriteBuffer(
        new UsefulCtrSramWriteReq,
        numEntries = WriteBufferSize,
        numPorts = 1,
        hasLookup = true,
        nameSuffix = s"tageUsefulTable${tableIdx}_${bankIdx}_${wayIdx}"
      )).suggestName(s"tage_useful_write_buffer_bank${bankIdx}_way${wayIdx}")
    }

  // read sram
  entrySram.zip(usefulCtrSram).zipWithIndex.foreach { case ((entryBank, usefulBank), bankIdx) =>
    val readValid = VecInit(
      io.readReq(0).valid && io.readReq(0).bits.bankMask(bankIdx),
      io.readReq(1).valid && io.readReq(1).bits.bankMask(bankIdx)
    )
    val readSetIdx = Mux(readValid(0), io.readReq(0).bits.setIdx, io.readReq(1).bits.setIdx)
    entryWriteBuffers(bankIdx).io.lookupSetIdx.get := RegEnable(readSetIdx, readValid.reduce(_ || _))
    usefulCtrWriteBuffers(bankIdx).foreach(
      _.io.lookupSetIdx.get := RegEnable(readSetIdx, readValid.reduce(_ || _))
    )

    entryBank.foreach { way =>
      way.io.r.req.valid       := readValid.reduce(_ || _)
      way.io.r.req.bits.setIdx := readSetIdx
    }
    usefulBank.foreach { way =>
      way.io.r.req.valid       := readValid.reduce(_ || _) && !usefulResetInFlightMask(bankIdx)
      way.io.r.req.bits.setIdx := readSetIdx
    }
    assert(!(readValid(0) && readValid(1)), s"read conflict in tage_table_${tableIdx}_bank_${bankIdx}")
  }

  // delay one cycle for better timing
  private val writeReqValid = RegNext(io.writeReq.valid, init = false.B)
  private val writeReq      = RegEnable(io.writeReq.bits, io.writeReq.valid)

  // write to write buffer
  entryWriteBuffers.zipWithIndex.foreach { case (buffer, bankIdx) =>
    buffer.io.write.zipWithIndex.foreach { case (bufferIn, wayIdx) =>
      val writeValid =
        writeReqValid && writeReq.bankMask(bankIdx) && writeReq.wayMask(wayIdx) && writeReq.writeEntryEn(wayIdx)
      bufferIn.valid       := writeValid
      bufferIn.bits.setIdx := writeReq.setIdx
      bufferIn.bits.entry  := writeReq.entries(wayIdx)
    }
    buffer.io.takenMask.get := writeReq.actualTakenMask
  }

  usefulCtrWriteBuffers.zipWithIndex.foreach { case (bankBuffer, bankIdx) =>
    bankBuffer.zipWithIndex.foreach { case (wayBuffer, wayIdx) =>
      val writeValid =
        writeReqValid && writeReq.bankMask(bankIdx) && writeReq.wayMask(wayIdx) && writeReq.writeUsefulEn(wayIdx)
      wayBuffer.io.write.head.valid          := writeValid
      wayBuffer.io.write.head.bits.setIdx    := writeReq.setIdx
      wayBuffer.io.write.head.bits.usefulCtr := writeReq.usefulCtrs(wayIdx)
    }
  }

  // write entry to sram from write buffer
  entrySram.zip(entryWriteBuffers).foreach { case (bank, buffer) =>
    bank.zip(buffer.io.read).foreach { case (way, bufferOut) =>
      way.io.w.apply(
        bufferOut.valid && !way.io.r.req.valid,
        bufferOut.bits.entry,
        bufferOut.bits.setIdx,
        1.U(1.W) // way mask
      )
      bufferOut.ready := way.io.w.req.ready && !way.io.r.req.valid
    }
  }

  usefulCtrSram.zip(usefulCtrWriteBuffers).zipWithIndex.foreach { case ((bank, bankBuffer), bankIdx) =>
    when(io.usefulResetStart) {
      usefulResetInFlightMask(bankIdx) := true.B
      usefulResetSetIdx(bankIdx)       := 0.U
    }.elsewhen(usefulResetInFlightMask(bankIdx) && bank.head.io.w.req.fire) {
      when(usefulResetSetIdx(bankIdx) === (NumSets - 1).U) {
        usefulResetInFlightMask(bankIdx) := false.B
        usefulResetSetIdx(bankIdx)       := 0.U
      }.otherwise {
        usefulResetSetIdx(bankIdx) := usefulResetSetIdx(bankIdx) + 1.U
      }
    }

    bank.zip(bankBuffer).foreach { case (way, wayBuffer) =>
      val usefulResetValid = usefulResetInFlightMask(bankIdx)
      val usefulWriteValid = wayBuffer.io.read.head.valid && !way.io.r.req.valid && !usefulResetValid
      way.io.w.apply(
        usefulResetValid || usefulWriteValid,
        Mux(usefulResetValid, UsefulCounter.Zero, wayBuffer.io.read.head.bits.usefulCtr),
        Mux(usefulResetValid, usefulResetSetIdx(bankIdx), wayBuffer.io.read.head.bits.setIdx),
        1.U(1.W) // way mask
      )
      wayBuffer.io.read.head.ready := way.io.w.req.ready && !way.io.r.req.valid && !usefulResetValid
    }
  }

  io.readResp.zipWithIndex.foreach { case (resp, i) =>
    val readBankMaskNext          = RegEnable(io.readReq(i).bits.bankMask, io.readReq(i).valid)
    val readDuringUsefulResetNext = RegEnable(readDuringUsefulReset(i), io.readReq(i).valid)

    val sramEntries = Mux1H(
      readBankMaskNext,
      entrySram.map(bank => VecInit(bank.map(way => way.io.r.resp.data.head)))
    )
    val shadowEntries = Mux1H(
      readBankMaskNext,
      entryWriteBuffers.map(buffer => VecInit(buffer.io.lookupResp.get.map(_.bits)))
    )
    val shadowHits = Mux1H(
      readBankMaskNext,
      entryWriteBuffers.map(buffer => VecInit(buffer.io.lookupResp.get.map(_.valid)))
    )
    resp.entries := VecInit((0 until NumWays).map { wayIdx =>
      Mux(shadowHits(wayIdx), shadowEntries(wayIdx).entry, sramEntries(wayIdx))
    })
    val sramUsefulCtrs = Mux1H(
      readBankMaskNext,
      usefulCtrSram.map(bank => VecInit(bank.map(way => way.io.r.resp.data.head)))
    )
    val shadowUsefulCtrs = Mux1H(
      readBankMaskNext,
      usefulCtrWriteBuffers.map(bank => VecInit(bank.map(_.io.lookupResp.get.head.bits.usefulCtr)))
    )
    val shadowUsefulHits = Mux1H(
      readBankMaskNext,
      usefulCtrWriteBuffers.map(bank => VecInit(bank.map(_.io.lookupResp.get.head.valid)))
    )
    val coherentUsefulCtrs = VecInit((0 until NumWays).map { wayIdx =>
      Mux(shadowUsefulHits(wayIdx), shadowUsefulCtrs(wayIdx), sramUsefulCtrs(wayIdx))
    })
    resp.usefulCtrs := Mux(
      readDuringUsefulResetNext,
      VecInit.fill(NumWays)(UsefulCounter.Zero),
      coherentUsefulCtrs
    )
  }

  io.sramResetDone := (
    entrySram.flatten.map(_.io.resetDone) ++ usefulCtrSram.flatten.map(_.io.resetDone)
  ).reduce(_ && _)

  XSPerfAccumulate("predict_read", io.readReq(0).valid)
  XSPerfAccumulate("train_read", io.readReq(1).valid)
  XSPerfAccumulate(
    "predict_write_buffer_forward",
    io.readReq(0).valid && entryWriteBuffers
      .zipWithIndex
      .map { case (buffer, bankIdx) =>
        io.readReq(0).bits.bankMask(bankIdx) && buffer.io.lookupResp.get.map(_.valid).reduce(_ || _)
      }
      .reduce(_ || _)
  )
  XSPerfAccumulate("write", io.writeReq.valid)
  XSPerfAccumulate(
    s"tage_write_entry_${tableIdx}",
    Mux(io.writeReq.valid, PopCount(io.writeReq.bits.writeEntryEn), 0.U)
  )
  XSPerfAccumulate(
    s"tage_write_useful_${tableIdx}",
    Mux(io.writeReq.valid, PopCount(io.writeReq.bits.writeUsefulEn), 0.U)
  )
  XSPerfAccumulate(s"tage_write_total_${tableIdx}", Mux(io.writeReq.valid, PopCount(io.writeReq.bits.wayMask), 0.U))
  XSPerfAccumulate(
    "drop_write",
    PopCount(entryWriteBuffers.flatMap(writePorts => writePorts.io.write.map(p => p.valid && !p.ready)))
  )
}
