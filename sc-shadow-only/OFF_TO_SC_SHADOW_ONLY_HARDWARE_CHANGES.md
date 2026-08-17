# OFF → ROB compression + SC shadow-only 하드웨어 변경 이관 문서

## 1. 문서 목적

이 문서는 현재 성능 선두 구성인 `RobCompressOnScShadowOnlyConfig`를 다른
개발 서버의 XiangShan 소스에 재적용하기 위한 하드웨어 변경 명세다.
공식 기준은 ROB compression이 꺼지고 기존 BPU training 경로를 사용하는
`OFF` 구성이다.

현재 실험 저장소:

```text
/home/joe/work/01_GEN2_COMP/rob-compress-readfree-training
```

실제 활성 구성:

```scala
RobCompressEn = true
TAGE ReadFreeTraining = false
SC ReadFreeTraining = true
```

중요: 현재 저장소의 working tree에는 과거 TAGE read-free 후보 코드도
존재한다. 이 구성에서는 해당 코드를 파라미터로 비활성화한다. 다른 서버로
옮길 때에는 단순히 현재 working-tree diff 전체를 적용하지 말고, 본 문서의
SC 관련 변경과 아래 명시한 ROB branch-isolation 변경만 가져가는 것이 좋다.

## 2. OFF와의 최상위 차이

| 영역 | OFF | SC shadow-only |
|---|---|---|
| ROB compression | 비활성 | 활성 |
| branch/jump ROB 압축 | 해당 없음 | 금지, 항상 독립 ROB entry |
| TAGE prediction/training | 기존 경로 | 기존 경로 유지 |
| TAGE read-free | 비활성 | 비활성 |
| SC prediction SRAM read | 기존 경로 | 기존 경로 유지 |
| SC training conflict | prediction과 동일 bank면 stall | 안전한 snapshot이면 training read 생략 |
| SC write buffer | drain 후 데이터 재사용 안 함 | 최신 row를 retained shadow로 보존 |
| SC SRAM 포트 | 기존 single-RW port | 동일 |
| SC metadata | 기존 prediction 결과 | 예측 당시 row 값, identity, version 추가 |
| stale/pending 처리 | 해당 없음 | repair/forward 또는 기존 read로 fallback |

즉 이 구성에는 서로 독립적인 두 변경이 포함된다.

1. branch-isolated ROB compression으로 backend ROB 압력을 낮춘다.
2. SC training이 prediction read와 충돌할 때 검증된 prediction snapshot을
   사용하여 single-port SRAM training read를 생략한다.

## 3. ROB compression 및 branch isolation

기반 커밋은 다음 두 개다.

```text
0f85391 experiment: enable ROB compression with instrumentation
72a51c2 experiment: isolate branches and jumps from ROB compression
```

관련 파일:

```text
src/main/scala/xiangshan/Parameters.scala
src/main/scala/xiangshan/backend/BackendParams.scala
src/main/scala/xiangshan/backend/CtrlBlock.scala
src/main/scala/xiangshan/backend/datapath/RFReadArbiter.scala
src/main/scala/xiangshan/backend/rename/CompressUnit.scala
src/main/scala/xiangshan/backend/rename/Rename.scala
src/main/scala/top/Configs.scala
```

`RobCompressEn=true`이면 기존 CompressUnit의 압축 가능한 연속 명령들이
하나의 ROB entry를 공유한다. 다만 `FuType.isBlockBackCompress`에 해당하는
branch/jump/control-flow 명령은 `canCompress`에서 제외한다.

필수 assertion:

- branch/jump는 반드시 `needRobFlags(i)=true`여야 한다.
- branch/jump entry의 `instrSizesVec(i)`는 1이어야 한다.
- branch/jump compression mask는 자기 자신만 선택해야 한다.

필수 성능 카운터:

```text
rename: rob_input_instr
rename: rob_allocated_entries
rename: rob_compressed_instr
rename: rob_entries_saved
rename: rob_control_flow_shared_entry   # 항상 0이어야 함
```

## 4. SC retained-shadow/read-free training

### 4.1 기존 OFF 동작

SC SRAM은 bank별 single-RW-port다. 같은 cycle에 prediction read와 committed
training read가 같은 bank를 요구하면 prediction이 우선하며 training은
`trainReady`를 낮추고 재시도한다. WriteBuffer는 SRAM write를 합치고
지연시키지만, drain된 최신 row를 training source로 조회하는 기능은 없다.

### 4.2 변경 동작 개요

prediction 시 실제로 사용한 coherent SC row를 metadata에 저장한다. 나중에
그 branch를 training할 때 다음 조건을 모두 만족하면 SRAM training read를
생략하고 snapshot을 old-state로 사용한다.

```text
SC update 필요
  AND prediction/training SRAM bank conflict
  AND snapshot identity == 현재 training identity
  AND snapshot version이 현재 row와 같거나 retained shadow로 repair 가능
  AND pipeline pending mutation을 최신 값으로 forward 가능
```

어느 조건이라도 충족하지 않으면 기존 SRAM + write-buffer coherent read
경로로 fallback한다. Prediction/redirect 결과를 계산하는 경로 자체는 기존과
같고, read-free 선택은 committed training 측에서만 수행한다.

## 5. 파일별 변경

### 5.1 `top/Configs.scala`

`RobCompressOnScShadowOnlyConfig`를 추가한다.

```scala
RobCompressEn = true
tageParameters.ReadFreeTraining = false
scParameters.ReadFreeTraining = true
```

빌드 banner에서 위 세 값을 반드시 출력해 잘못된 configuration을 검출한다.

### 5.2 `bpu/sc/Parameters.scala`

`ScParameters`에 다음 파라미터를 추가한다.

```scala
ReadFreeTraining: Boolean = false
```

row version 폭을 정의한다.

```scala
ScRowVersionWidth = 12
```

기본값은 false로 두어 다른 configuration 동작을 바꾸지 않는다.

### 5.3 `bpu/sc/Bundles.scala`

`ScMeta`에 prediction 당시 coherent SC 상태를 추가한다.

- Path table별 모든 way counter bits
- Global table별 모든 way counter bits
- Backward table별 모든 way counter bits
- IMLI table 모든 way counter bits
- Bias table 모든 way counter bits
- 각 table row의 12-bit version
- prediction bank mask
- prediction 당시 각 table index(identity 검사용)

`ScTableSramWriteReq`에는 `baseVersion`을 추가한다. 이는 retained write-buffer
entry가 어느 row version부터의 mutation을 포함하는지 나타낸다.

주의: 이 metadata는 FTQ/BPU metadata 저장 폭을 증가시킨다. 다른 서버에서
합성할 때 SRAM macro 포트 수뿐 아니라 FTQ metadata 면적과 배선 timing도
반드시 확인해야 한다.

### 5.4 `bpu/WriteBuffer.scala`

기존 WriteBuffer에 선택적 lookup 인터페이스를 추가한다.

```scala
hasLookup
lookupSetIdx
lookupResp
hasSecondLookup
secondLookupSetIdx
secondLookupResp
```

핵심 변경은 SRAM으로 drain된 entry도 `valid` 상태로 보존한다는 것이다.
따라서 buffer가 단순 pending-write FIFO 역할뿐 아니라 최근 row의 coherent
retained shadow 역할도 한다.

동일 port row 안에는 같은 `setIdx`의 retained entry가 최대 하나만 존재해야
한다. replacement 시 동일 setIdx victim을 우선 선택하며, lookup hit가 둘
이상이면 `XSError`를 발생시킨다.

### 5.5 `bpu/sc/ScTable.scala`

각 SC table/bank에 존재하던 WriteBuffer를 그대로 사용하되 SC read-free가
활성일 때 lookup 두 개를 노출한다.

- prediction/training coherent read overlay용 lookup
- stale snapshot repair용 second lookup

각 bank/set row에 12-bit `rowVersions`를 추가한다. 논리적 update가
WriteBuffer에 accept될 때 version을 증가시키며, SRAM drain 시점이 아니다.
따라서 version은 SRAM 물리 상태가 아니라 SRAM + retained write-buffer를
합친 논리 상태를 나타낸다.

SRAM read 결과 위에 retained `wayMask/wayData`를 overlay하여 coherent row를
생성한다. SRAM macro와 포트 구성은 변경하지 않는다.

추가 I/O 개념:

```text
predictVersion / trainVersion / commitVersion
repairSetIdx / repairBankMask
repairHit / repairBaseVersion / repairWayMask / repairEntries
```

### 5.6 `bpu/sc/Sc.scala`

prediction 단계에서 coherent SC entry 값, row version, bank/index identity를
`ScMeta`에 기록한다.

training t0에서 다음을 검사한다.

1. 실제 SC counter update가 필요한가?
2. 현재 prediction read와 같은 bank인가?
3. 저장된 bank 및 모든 table index가 training 계산값과 같은가?
4. snapshot version이 현재 version과 같은가?
5. 다르면 retained shadow가 `[baseVersion, currentVersion]` 구간을 완전히
   덮어 repair 가능한가?

fresh row는 snapshot을 그대로 쓰고, shadow-covered row는 retained way들을
overlay해 최신 logical row로 repair한다.

training pipeline t1/t2의 동일 bank/row update도 비교해 더 최신 pending
counter 값을 forward한다. 이를 위해 최근 두 pipeline stage의 bank와 각
table index를 reservation으로 추적한다.

최종 선택:

```scala
t0_useReadFreeSnapshot =
  ScReadFreeTraining && bankConflict && identityMatch && snapshotFresh

t0_needRead = t0_needWrite && !t0_useReadFreeSnapshot
trainReady = !unavoidedBankConflict
```

t2 commit 직전에 snapshot version과 각 table의 `commitVersion`을 다시
비교한다. 더 최신 update가 끼어들었다면 해당 read-free update write를
허용하지 않고 `read_free_commit_stale_drop`을 증가시킨다. 현재 모든 검증
워크로드에서 이 값은 0이었다.

주의: 장기 sign-off 설계에서는 stale update를 단순히 skip하는 것보다
assertion으로 발생을 금지하거나 기존 read 경로로 replay하는 편이 더 강한
보장이다. 다른 서버에서 후속 수정 시 이 부분을 우선 검토해야 한다.

## 6. 데이터 흐름

```text
Prediction SC SRAM read
        │
        ├─ retained WB overlay ──> coherent SC row ──> 기존 SC prediction
        │                                      │
        │                                      └─> ScMeta snapshot/version/identity
        │
Committed training t0
        │
        ├─ bank conflict 없음 ──> 기존 training SRAM read
        │
        └─ bank conflict 있음
                │
                ├─ identity/version 불충분 ──> 기존 read로 fallback/stall
                │
                └─ snapshot 안전
                        ├─ retained shadow repair
                        ├─ t1/t2 pending update forward
                        └─ SRAM read 없이 counter 계산
                                      │
                                      └─ t2 version 재검사 후 기존 WB write
```

## 7. 추가 하드웨어 자원

추가되는 자원:

- SC prediction metadata의 entry snapshots, versions, bank/index identity
- SC row별 12-bit version register array
- retained WriteBuffer lookup 비교기와 way overlay mux
- 두 pipeline stage의 pending bank/index registers
- repair/forward/version 비교 조합 논리
- 디버그 및 성능 카운터

추가되지 않는 자원:

- SC SRAM read/write port
- TAGE SRAM port
- 별도 SC SRAM 복제본
- training operation queue
- prediction pipeline stage

기존 검증에서 SC SRAM macro는 branch-isolated 구성과 byte-identical했고,
두 single-RW-port macro 구성이 유지됐다. 그러나 metadata/register 면적과
lookup mux timing은 별도 합성 확인이 필요하다.

## 8. 필수 카운터와 안전 조건

```text
bpu.sc: read_free_conflict_candidate
bpu.sc: read_free_snapshot_train
bpu.sc: read_free_shadow_repair
bpu.sc: read_free_identity_fallback
bpu.sc: read_free_stale_fallback
bpu.sc: read_free_pending_fallback
bpu.sc: read_free_pending_forward
bpu.sc: read_free_commit_stale_drop
bpu.tage: read_conflict
bpu: train_stall
frontend: branchMispredicts
rename: rob_entries_saved
rename: rob_control_flow_shared_entry
```

장기 실행의 필수 조건:

```text
read_free_commit_stale_drop == 0
rob_control_flow_shared_entry == 0
WriteBuffer duplicate lookup assertion == 0
NEMU/difftest mismatch == 0
BAD/CRITICAL trap == 0
```

## 9. 현재 측정 결과

| 워크로드 | Branch-isolated 대비 IPC | OFF 대비 IPC | stale drop |
|---|---:|---:|---:|
| bzip2-derived | +2.441% | +6.683% | 0 |
| libquantum 934K | +1.610% | +4.694% | 0 |
| heat-stress 998K | -0.062% | 미측정 | 0 |

확장 heat-stress의 -0.062%는 6개 추가 오예측에 의한 104-cycle 차이이며,
약 1M 명령어에서 지속적인 throughput 퇴행은 관찰되지 않았다.

## 10. 이관 및 재구축 체크리스트

1. 대상 XiangShan revision을 현재 기반 revision과 맞춘다.
2. ROB compression 커밋 `0f85391`, branch isolation 커밋 `72a51c2`를 먼저
   적용하거나 동일 기능을 대상 revision에 수동 이식한다.
3. SC 관련 여섯 파일의 변경만 이식한다.
4. `RobCompressOnScShadowOnlyConfig`에서 TAGE read-free=false를 확인한다.
5. `git diff --check`와 Scala compile을 수행한다.
6. 메모리를 고려해 RTL elaboration과 Verilator build는 `-j1`로 수행한다.
7. 생성된 configuration banner 세 값을 확인한다.
8. SC SRAM macro 목록, port 수와 baseline hash를 비교한다.
9. 짧은 heat, bzip2-derived, 확장 heat, libquantum 순으로 difftest한다.
10. 위 안전 카운터가 모두 0인지 확인한 뒤 ZeBu 장기 실행으로 넘어간다.

관련 구현 파일:

```text
src/main/scala/top/Configs.scala
src/main/scala/xiangshan/frontend/bpu/WriteBuffer.scala
src/main/scala/xiangshan/frontend/bpu/sc/Bundles.scala
src/main/scala/xiangshan/frontend/bpu/sc/Parameters.scala
src/main/scala/xiangshan/frontend/bpu/sc/Sc.scala
src/main/scala/xiangshan/frontend/bpu/sc/ScTable.scala
```

관련 결과 문서:

```text
results/sc-shadow-only/RESULTS.md
results/heat-calc/extended-256/RESULTS.md
/home/joe/work/specint2006-investigation/results/libquantum-rtl-1h/RESULTS.md
```

