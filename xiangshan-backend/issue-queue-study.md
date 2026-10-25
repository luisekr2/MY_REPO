# Issue Queue Study

명령이 IQ에 들어가 실행될 때까지의 흐름을 여섯 질문으로 살펴봅니다. 먼저 전체 구조를 보고, 명령 분배·의존성·실패 사례를 거쳐 처리량을 제한하는 조건을 설명합니다.

XiangShan Kunminghu V3 · DefaultConfig(1)<br>
기준 커밋: f85a1f71da8c1522035744c71e2cb266bfed1a7d<br>
예제의 순위·중재 승리·입력 상태는 설명용 가정입니다. 실제 cycle trace와 성능 측정 결과는 포함하지 않습니다.

<a id="contents"></a>

## 읽는 순서

1. [명령은 왜 IQ에서 기다릴까?](#q1)
2. [13개 IQ는 어디로 명령을 보낼까?](#q2)
3. [독립적인 ADD 8개는 어떻게 나뉠까?](#q3)
4. [기다리던 명령은 언제 선택 후보가 될까?](#q4)
5. [선택했는데 PRF 포트를 얻지 못하면?](#q5)
6. [ALU가 남아 있는데도 왜 멈출까?](#q6)

[참고 자료: 상세 조건과 소스 근거](#references)

<a id="q1"></a>

## 1. 명령은 왜 IQ에서 기다릴까?

![Dispatch에서 실행까지 IQ의 위치](assets/issue-queue-study/figure-01.png)

**그림 1.** 기능 흐름을 단순화한 그림. 화살표는 cycle 단위가 아니다. 예상 wakeup 경로는 참고 D에서 설명한다.

[SVG 원본](assets/issue-queue-study/figure-01.svg)

**명령열 예제**

```text
U0: add x5, x1, x2
```

```text
U1: sub x6, x5, x3
```

U1은 IQ에 들어간 뒤 U0가 만드는 x5의 준비 정보를 기다릴 수 있다.

**핵심 결론**

IQ는 미준비 명령을 보관하고, 실행 후보가 된 명령을 선택한다.

[상세 조건·소스 근거 ↗](#ref-position)

[목차로 돌아가기](#contents)

<a id="q2"></a>

## 2. 13개 IQ는 어디로 명령을 보낼까?

![13개 IQ를 실행 경로별 다섯 묶음으로 보기](assets/issue-queue-study/figure-02.png)

**그림 2.** 중간의 DataPath·BypassNetwork 생략. 메모리 IQ는 Int Region, 실행 본체는 MemBlock에 있다.

[SVG 원본](assets/issue-queue-study/figure-02.svg)

**명령열 예제**

```text
IQ0: add x5, x1, x2 → ALU0
```

```text
IQ0: beq x3, x4, L → BJU0
```

서로 다른 두 명령이 조건을 만족하면 두 출력을 함께 사용할 수 있다.

**핵심 결론**

13개 IQ에 16개 발행 경로가 있다. IQ에 들어간 뒤의 실행 목적지는 고정된다.

[상세 조건·소스 근거 ↗](#ref-topology)

[목차로 돌아가기](#contents)

<a id="q3"></a>

## 3. 독립적인 ADD 8개는 어떻게 나뉠까?

![ADD 여덟 개를 순위 목록의 여섯 IQ에 분배](assets/issue-queue-study/figure-03.png)

**그림 3.** A: 설명용 후보 순위. 현재 occupancy를 즉시 정렬한 RTL 파형이 아니다.

[SVG 원본](assets/issue-queue-study/figure-03.svg)

**명령열 예제**

```text
A0…A7: add x10…x17, x1, x2
```

순서대로 IQ4, IQ0, IQ5, IQ2, IQ1, IQ3, IQ4, IQ0를 선택한다.<br>
IQ4·IQ0에 두 개씩 enqueue되어도 각 ALU 출력의 issue 폭은 하나다.

**핵심 결론**

실행 가능한 후보를 먼저 정하고, 같은 FU 그룹의 명령을 후보 순위에 따라 나눈다.

[상세 조건·소스 근거 ↗](#ref-dispatch)

[목차로 돌아가기](#contents)

<a id="q4"></a>

## 4. 기다리던 명령은 언제 선택 후보가 될까?

![의존성이 있는 명령이 대기에서 선택 후보가 되는 과정](assets/issue-queue-study/figure-04.png)

**그림 4.** A: x5의 physical destination을 p41로 가정. 사건 순서이며 고정 latency 표가 아니다.

[SVG 원본](assets/issue-queue-study/figure-04.svg)

**명령열 예제**

```text
U0: add x5, x1, x2 / U1: sub x6, x5, x3
```

```text
U2: xor x7, x8, x9 // U0와 독립
```

U1은 x5의 준비 정보를 기다린다. U2는 조건이 맞으면 U1보다 먼저 선택될 수 있다.

**핵심 결론**

source 준비는 선택의 시작 조건이다. busy·FU·선택 우선순위와 operand 수용도 필요하다.

[상세 조건·소스 근거 ↗](#ref-ready)

[목차로 돌아가기](#contents)

<a id="q5"></a>

## 5. 선택했는데 PRF 포트를 얻지 못하면?

![같은 port와 bank의 충돌이 IQ 재선택으로 돌아오는 경로](assets/issue-queue-study/figure-05.png)

**그림 5.** A: 두 요청 모두 PRF 필요, V가 중재 승리. bank(p5)=bank(p9)=1. 재시도 간격은 가변.

[SVG 원본](assets/issue-queue-study/figure-05.svg)

**명령열 예제**

```text
U: add x10, x2, x3 → IQ3 / ALU3, psrc1=p5
```

```text
V: ld x11, 0(x4) → IQ6 / LDU0, psrc0=p9
```

U는 실패 응답을 받아 같은 IQ에서 재선택된다. Rename·ROB 할당은 유지된다.

**핵심 결론**

IQ 선택 뒤에도 발행이 실패할 수 있다. 보존된 entry가 실패 응답을 받아 재시도한다.

[상세 조건·소스 근거 ↗](#ref-retry)

[목차로 돌아가기](#contents)

<a id="q6"></a>

## 6. ALU가 남아 있는데도 왜 멈출까?

![하나의 IQ가 막힐 때 뒤쪽 명령까지 기다리는 이유](assets/issue-queue-study/figure-06.png)

**그림 6.** A: Q3의 후보 순위를 사용. IQ0 미수용 외의 gate는 허용한다고 가정한다.

[SVG 원본](assets/issue-queue-study/figure-06.svg)

**명령열 예제**

```text
A0…A3: add x10…x13, x1, x2
```

A0만 통과하고 A1–A3는 대기한다. 다음 선택에서 목적지가 달라질 수 있지만,<br>
그 cycle 안에 다른 빈 IQ로 즉시 우회하는 동작은 아니다.

**핵심 결론**

처리량은 배치·IQ 수용·고정 발행 경로·operand 공급이 함께 결정한다.

[상세 조건·소스 근거 ↗](#ref-bottleneck)

[목차로 돌아가기](#contents)

<a id="references"></a>

## 참고 자료: 상세 조건과 소스 근거

본문은 설명 흐름을 위한 축약이다. 아래 조건과 고정 커밋의 링크를 함께 읽는다. 예제의 명령 배치·중재 승리·wakeup 시점은 명시한 가정이며, 실제 cycle trace가 아니다.

[A. 단계 경계와 용어](#ref-position)[B. 13개 IQ의 전체 대응](#ref-topology)[C. 후보·점수·분배식](#ref-dispatch)[D. 준비·선택·wakeup 조건](#ref-ready)[E. PRF 충돌부터 재발행까지](#ref-retry)[F. 정지·재선택·확장 검토](#ref-bottleneck)

<a id="ref-position"></a>

### A. 단계 경계와 용어

[본문 1절로 돌아가기](#q1)

Dispatch는 source가 미준비여도 자원이 허용하면 IQ에 넣는다. IQ는 operand 값 전체를 보관하는 PRF가 아니다. 선택 뒤에도 operand 공급과 실행기 수용이 필요하다는 점만 먼저 설명한다.

**F · 소스 사실.** IQ select는 deqBeforeDly에서 선택하고 deqDelay에 등록한다. 다음 OG0에서 read grant, writeback 충돌, forward cancel을 검사한다. OG1은 PRF 데이터·bypass를 선택하고 downstream 수용을 확인한다. EXU 입력에도 별도 register 경계가 있다.

OG1의 일반 정수 finalSuccess는 IQ의 해당 시도를 마치는 조건이다. 연산 결과의 writeback, ROB 완료, architectural commit과는 서로 다른 사건이다. 본문 1절 그림은 기능 연결을 나타내며 cycle 합산용이 아니다.

#### 소스 근거

- [Region.scala:40–65](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/Region.scala#L40-L65)
- [Region.scala:475–525](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/Region.scala#L475-L525)
- [Region.scala:536–575](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/Region.scala#L536-L575)
- [Region.scala:43–59](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/Region.scala#L43-L59)
- [DataPath.scala:51–64](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/datapath/DataPath.scala#L51-L64)
- [DataPath.scala:287–307](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/datapath/DataPath.scala#L287-L307)
- [Regfile.scala:314–326](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/regfile/Regfile.scala#L314-L326)
- [RegCache.scala:42–58](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/regcache/RegCache.scala#L42-L58)
- [IssueQueue.scala:895–925](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/issue/IssueQueue.scala#L895-L925)
- [DataPath.scala:519–549](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/datapath/DataPath.scala#L519-L549)
- [DataPath.scala:604–643](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/datapath/DataPath.scala#L604-L643)
- [Region.scala:509–559](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/Region.scala#L509-L559)
- [NewPipelineConnect.scala:41–51](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/datapath/NewPipelineConnect.scala#L41-L51)

<a id="ref-topology"></a>

### B. 13개 IQ의 전체 대응

[본문 2절로 돌아가기](#q2)

BJU는 두 번째 ALU가 아니다. IQ0의 ADD 두 개를 ALU0와 BJU0에 나눠 보낼 수 없다. ALU0라는 EXU 내부에는 여러 FU가 있으나 dequeue 수는 그대로다.

| IQ | 개별 entry 구성 | 개별 enq / deq | 목적지 |
| --- | --- | --- | --- |
| 0, 1, 2 | 18 = 2 + 6 + 10 | 2 / 2 | 각 ALU0/1/2 및 BJU0/1/2 |
| 3, 4, 5 | 20 = 2 + 6 + 12 | 2 / 1 | 각 ALU3/4/5 |
| 6, 7, 8 | 20 = 2 + 6 + 12 | 2 / 1 | 각 LDU0/1/2 |
| 9, 10 | 16 = 2 + 2 + 12 | 2 / 1 | 각 STA0/1 |
| 11, 12 | 16 = 2 + 2 + 12 | 2 / 1 | 각 STD0/1 |

**C · 소스 구성에서 계산.** 238 entries, 26 enqueue 위치, 16 dequeue 위치. enqueue 위치의 합은 8-wide Dispatch가 매 cycle 수용하는 명령 수가 아니다. entry 구성은 enqueue + simple + complex 순서다.

**F.** IQ0–2의 ALU/BJU 출력은 서로 다른 FU 집합에서 서로 다른 uop를 선택한다. 하나의 uop를 두 출력에 복제하는 구조가 아니다. 메모리 경로 7개는 ExuBlock의 로컬 EXU 9개와 실행 위치가 다르다.

#### 소스 근거

- [Parameters.scala:342–452](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/Parameters.scala#L342-L452)
- [Region.scala:40–56](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/Region.scala#L40-L56)
- [IssueBlockParams.scala:165–165](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/issue/IssueBlockParams.scala#L165-L165)
- [IssueBlockParams.scala:346–354](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/issue/IssueBlockParams.scala#L346-L354)
- [ExuBlock.scala:19–45](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/exu/ExuBlock.scala#L19-L45)
- [Region.scala:475–549](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/Region.scala#L475-L549)
- [Entries.scala:46–61](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/issue/Entries.scala#L46-L61)
- [Entries.scala:115–145](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/issue/Entries.scala#L115-L145)
- [Parameters.scala:342–446](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/Parameters.scala#L342-L446)
- [IssueBlockParams.scala:167–173](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/issue/IssueBlockParams.scala#L167-L173)
- [Entries.scala:23–52](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/issue/Entries.scala#L23-L52)
- [EntryBundles.scala:26–44](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/issue/EntryBundles.scala#L26-L44)
- [EntryBundles.scala:303–314](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/issue/EntryBundles.scala#L303-L314)
- [EntryBundles.scala:475–477](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/issue/EntryBundles.scala#L475-L477)
- [IssueQueue.scala:425–467](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/issue/IssueQueue.scala#L425-L467)
- [IssueQueue.scala:534–588](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/issue/IssueQueue.scala#L534-L588)
- [FuBusyTableWrite.scala:12–60](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/issue/FuBusyTableWrite.scala#L12-L60)
- [FuBusyTableRead.scala:7–37](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/issue/FuBusyTableRead.scala#L7-L37)
- [IssueQueue.scala:604–620](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/issue/IssueQueue.scala#L604-L620)
- [Entries.scala:32–44](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/issue/Entries.scala#L32-L44)
- [Entries.scala:148–152](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/issue/Entries.scala#L148-L152)
- [Entries.scala:252–255](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/issue/Entries.scala#L252-L255)
- [EntryBundles.scala:210–240](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/issue/EntryBundles.scala#L210-L240)
- [EntryBundles.scala:453–461](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/issue/EntryBundles.scala#L453-L461)
- [IssueQueue.scala:895–925](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/issue/IssueQueue.scala#L895-L925)

<a id="ref-dispatch"></a>

### C. 후보·점수·분배식

[본문 3절로 돌아가기](#q3)

먼저 FU eligibility가 IQ 후보를 제한한다. 점수는 lane별 대기량의 추정치이고 등록·보정된다. 최종 ready와 누적 enqueue 개수 검사는 별도다. 여덟 ADD의 동시 수용은 모든 gate가 열렸을 때의 조건부 사례다.

| 명령 종류 | 후보 IQ |
| --- | --- |
| ADD / XOR 등 AluCfg | 0–5 |
| 조건분기 | 0–2 |
| MUL / DIV | 4–5 / 1 |
| CSR · Fence · Link / I2f / Bku · VSet | 0 / 2 / 3 |
| Scalar load / store 주소 | 6–8 / 9–10 |

**F.** 기준 설정은 EnableDispatchIQBalanceOpt=true. lane별 점수는 IQ 전체 빈칸 수와 같지 않다. Q는 해당 dequeue FU에 맞는 valid count에서 capped issued count와 OG1-final의 deqBeforeDly.valid 예상 출구를 뺀 값이다. 선택 중인 A를 더해 Qreg=RegNext(Q+A), score=Qreg+A를 만든다. A는 fire만 세는 값이 아니다.

전역 순위를 등록하고 저장된 순위를 3개씩 나눠 보정한다. updateInterval=3은 이 segment 크기다. 같은 FU 그룹 앞선 valid 명령 수 k에 대해 orderedCandidates\[k % N\]을 선택한다. 현재 ready는 순위에서 후보를 제거하는 mask가 아니다.

**A/C · 본문 3절.** 순위 \[4,0,5,2,1,3\]을 가정하면 k=0…7의 목적지는 \[4,0,5,2,1,3,4,0\]다. 모든 IQ ready와 ROB·LSQ·순서 gate 통과를 가정한다.

**Store 예외 경로.** STD는 독립 Dispatch 후보가 아니다. STA0의 enqueue는 STD0로, STA1은 STD1로 복제한다. 선택 pair의 ready는 staReady && stdReady && vstdReady이며 주소·데이터 issue는 이후 별도로 진행된다.

#### 소스 근거

- [Dispatch.scala:53–108](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/dispatch/Dispatch.scala#L53-L108)
- [Dispatch.scala:551–579](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/dispatch/Dispatch.scala#L551-L579)
- [Dispatch.scala:611–633](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/dispatch/Dispatch.scala#L611-L633)
- [Parameters.scala:342–446](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/Parameters.scala#L342-L446)
- [Parameters.scala:121–128](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/Parameters.scala#L121-L128)
- [Configs.scala:498–515](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/top/Configs.scala#L498-L515)
- [IssueQueue.scala:1000–1059](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/issue/IssueQueue.scala#L1000-L1059)
- [Region.scala:828–828](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/Region.scala#L828-L828)
- [Backend.scala:240–245](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/Backend.scala#L240-L245)
- [CtrlBlock.scala:807–807](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/CtrlBlock.scala#L807-L807)
- [Dispatch.scala:432–445](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/dispatch/Dispatch.scala#L432-L445)
- [Dispatch.scala:446–579](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/dispatch/Dispatch.scala#L446-L579)
- [Dispatch.scala:515–583](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/dispatch/Dispatch.scala#L515-L583)
- [Dispatch.scala:611–648](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/dispatch/Dispatch.scala#L611-L648)
- [Parameters.scala:345–446](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/Parameters.scala#L345-L446)
- [Dispatch.scala:53–56](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/dispatch/Dispatch.scala#L53-L56)
- [Region.scala:123–131](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/Region.scala#L123-L131)
- [Region.scala:268–310](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/Region.scala#L268-L310)

<a id="ref-ready"></a>

### D. 준비·선택·wakeup 조건

[본문 4절로 돌아가기](#q4)

본문에서는 source readiness의 역할에 집중한다. 실제 선택은 partition 우선순위와 partition 내부 age를 적용한다. complex entry의 same-cycle wakeup bypass와 speculative wakeup 취소는 참고에서 분리한다.

**F.** Status.canIssue는 srcReady && !issued && !blocked에서 시작하며 valid·flush와 FU/WB busy mask를 더 검사한다. 현재 정수 구성의 일반 선택은 complex, simple, enqueue partition 우선순위를 적용하고 각 partition 안에서 age를 사용한다. 전체 IQ에서 언제나 가장 오래된 uop 하나를 고르는 모델은 정확하지 않다.

Complex entry는 같은 cycle의 IQ wakeup을 canIssueBypass로 반영할 수 있다. Simple entry에는 같은 조합 bypass 선택이 없다. 따라서 readiness가 실제 결과 전달보다 앞설 수 있으며, 예상 wakeup이 취소되면 준비 상태나 소비자의 시도도 되돌려야 한다.

**A · 본문 4절.** x5→p41은 설명용 rename 결과다. U2가 먼저 선택되는 사례는 source 준비, 실행 경로, busy, age와 operand 수용이 모두 허용될 때만 성립한다. 그림의 사건 간격은 실제 cycle 수를 뜻하지 않는다.

#### 소스 근거

- [Entries.scala:46–61](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/issue/Entries.scala#L46-L61)
- [Entries.scala:115–145](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/issue/Entries.scala#L115-L145)
- [Parameters.scala:342–446](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/Parameters.scala#L342-L446)
- [IssueBlockParams.scala:167–173](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/issue/IssueBlockParams.scala#L167-L173)
- [Entries.scala:23–52](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/issue/Entries.scala#L23-L52)
- [EntryBundles.scala:26–44](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/issue/EntryBundles.scala#L26-L44)
- [EntryBundles.scala:303–314](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/issue/EntryBundles.scala#L303-L314)
- [EntryBundles.scala:475–477](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/issue/EntryBundles.scala#L475-L477)
- [IssueQueue.scala:425–467](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/issue/IssueQueue.scala#L425-L467)
- [IssueQueue.scala:534–588](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/issue/IssueQueue.scala#L534-L588)
- [FuBusyTableWrite.scala:12–60](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/issue/FuBusyTableWrite.scala#L12-L60)
- [FuBusyTableRead.scala:7–37](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/issue/FuBusyTableRead.scala#L7-L37)
- [IssueQueue.scala:604–620](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/issue/IssueQueue.scala#L604-L620)
- [Entries.scala:32–44](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/issue/Entries.scala#L32-L44)
- [Entries.scala:148–152](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/issue/Entries.scala#L148-L152)
- [Entries.scala:252–255](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/issue/Entries.scala#L252-L255)
- [EntryBundles.scala:210–240](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/issue/EntryBundles.scala#L210-L240)
- [EntryBundles.scala:453–461](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/issue/EntryBundles.scala#L453-L461)
- [IssueQueue.scala:895–925](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/issue/IssueQueue.scala#L895-L925)
- [DataPath.scala:106–118](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/datapath/DataPath.scala#L106-L118)
- [RFReadArbiter.scala:99–104](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/datapath/RFReadArbiter.scala#L99-L104)
- [RFReadArbiter.scala:210–220](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/datapath/RFReadArbiter.scala#L210-L220)
- [DataPath.scala:614–643](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/datapath/DataPath.scala#L614-L643)
- [DataPath.scala:658–664](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/datapath/DataPath.scala#L658-L664)
- [Region.scala:150–164](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/Region.scala#L150-L164)
- [EntryBundles.scala:217–240](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/issue/EntryBundles.scala#L217-L240)
- [EntryBundles.scala:41–44](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/issue/EntryBundles.scala#L41-L44)
- [IssueQueue.scala:425–460](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/issue/IssueQueue.scala#L425-L460)
- [DataPath.scala:658–691](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/datapath/DataPath.scala#L658-L691)
- [DataPath.scala:624–643](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/datapath/DataPath.scala#L624-L643)
- [DataPath.scala:666–685](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/datapath/DataPath.scala#L666-L685)
- [EntryBundles.scala:341–346](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/issue/EntryBundles.scala#L341-L346)
- [EntryBundles.scala:385–389](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/issue/EntryBundles.scala#L385-L389)
- [IssueBlockParams.scala:237–252](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/issue/IssueBlockParams.scala#L237-L252)
- [Region.scala:175–199](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/Region.scala#L175-L199)
- [Region.scala:559–575](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/Region.scala#L559-L575)
- [IssueQueue.scala:117–134](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/issue/IssueQueue.scala#L117-L134)
- [IssueQueue.scala:176–178](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/issue/IssueQueue.scala#L176-L178)
- [IssueQueue.scala:757–766](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/issue/IssueQueue.scala#L757-L766)
- [DataPath.scala:596–600](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/datapath/DataPath.scala#L596-L600)
- [DataPath.scala:625–643](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/datapath/DataPath.scala#L625-L643)
- [DataPath.scala:688–691](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/datapath/DataPath.scala#L688-L691)
- [DataPath.scala:49](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/datapath/DataPath.scala#L49)
- [DataPath.scala:666–691](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/datapath/DataPath.scala#L666-L691)
- [DataPath.scala:701–706](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/datapath/DataPath.scala#L701-L706)
- [IssueQueue.scala:935–961](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/issue/IssueQueue.scala#L935-L961)

<a id="ref-retry"></a>

### E. PRF 충돌부터 재발행까지

[본문 5절로 돌아가기](#q5)

같은 bank만으로 충돌하는 것은 아니다. 같은 logical read port와 같은 bank가 겹친다. 중재 승자는 예제의 가정이고 configured priority 0만으로 고정 승자를 설명하지 않는다. 소비자 cancel과 직접 failed 응답을 구분한다.

1. ALU3 src1과 LDU0 src0은 논리 read port 7을 공유한다. psrc 하위 2bit가 bank를 고르므로 p5와 p9는 모두 bank 1이다.
2. 중재에서 필요한 operand 하나라도 ready를 받지 못하면 rdNotBlock이 내려가고 s0.ready가 0이 될 수 있다. 부분 grant만으로 uop를 수락하지 않는다.
3. og0resp.failed = fromIQ.valid && !fromIQ.ready. Region은 각 dequeue의 직접 응답을 해당 IQ로 돌려준다.
4. entry는 issueTimer와 deqPortIdx로 응답을 고른다. 실패 시 issued를 내리고 valid와 payload를 보존한다.
5. 준비·busy·FU·선택 조건이 다시 맞으면 같은 entry가 재시도한다. 고정 retry cycle은 보장하지 않는다.

**A.** 본문은 V가 승리하는 등록된 중재 상태와 다른 필요한 자원 통과를 가정한다. configured priority 숫자만으로 영구 승자를 정하지 않는다. PRF 충돌 자체가 source 준비 상태를 항상 0으로 만드는 것도 아니다.

**경계.** 일반 정수는 OG1 응답, LDU는 snResp, STA/STD는 별도 memory feedback 단계가 최종 성공을 결정한다. load가 MemBlock에 수락된 뒤의 miss replay와 IQ 재발행은 소유자가 다르다. redirect는 flush 대상 entry를 제거한다.

**소스 주의점.** 이 커밋의 og1Cancel broadcast 식은 valid 게이트와 함께 2-state로 전개하면 0이 되는 것으로 관찰된다. OG1 실패 처리는 직접 og1resp.failed와 wakeup queue flush 연결을 기준으로 설명했다. 최적화 RTL·설계 의도는 추가 검증 대상이다.

#### 소스 근거

- [DataPath.scala:106–118](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/datapath/DataPath.scala#L106-L118)
- [RFReadArbiter.scala:99–104](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/datapath/RFReadArbiter.scala#L99-L104)
- [RFReadArbiter.scala:210–220](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/datapath/RFReadArbiter.scala#L210-L220)
- [DataPath.scala:614–643](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/datapath/DataPath.scala#L614-L643)
- [DataPath.scala:658–664](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/datapath/DataPath.scala#L658-L664)
- [Region.scala:150–164](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/Region.scala#L150-L164)
- [Entries.scala:32–44](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/issue/Entries.scala#L32-L44)
- [Entries.scala:252–255](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/issue/Entries.scala#L252-L255)
- [EntryBundles.scala:217–240](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/issue/EntryBundles.scala#L217-L240)
- [EntryBundles.scala:453–461](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/issue/EntryBundles.scala#L453-L461)
- [EntryBundles.scala:41–44](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/issue/EntryBundles.scala#L41-L44)
- [IssueQueue.scala:425–460](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/issue/IssueQueue.scala#L425-L460)
- [DataPath.scala:658–691](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/datapath/DataPath.scala#L658-L691)
- [DataPath.scala:624–643](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/datapath/DataPath.scala#L624-L643)
- [DataPath.scala:666–685](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/datapath/DataPath.scala#L666-L685)
- [EntryBundles.scala:210–240](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/issue/EntryBundles.scala#L210-L240)
- [EntryBundles.scala:341–346](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/issue/EntryBundles.scala#L341-L346)
- [EntryBundles.scala:385–389](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/issue/EntryBundles.scala#L385-L389)
- [IssueBlockParams.scala:237–252](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/issue/IssueBlockParams.scala#L237-L252)
- [Region.scala:175–199](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/Region.scala#L175-L199)
- [Region.scala:559–575](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/Region.scala#L559-L575)
- [IssueQueue.scala:117–134](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/issue/IssueQueue.scala#L117-L134)
- [IssueQueue.scala:176–178](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/issue/IssueQueue.scala#L176-L178)
- [IssueQueue.scala:757–766](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/issue/IssueQueue.scala#L757-L766)
- [EntryBundles.scala:303–314](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/issue/EntryBundles.scala#L303-L314)
- [DataPath.scala:596–600](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/datapath/DataPath.scala#L596-L600)
- [DataPath.scala:625–643](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/datapath/DataPath.scala#L625-L643)
- [DataPath.scala:688–691](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/datapath/DataPath.scala#L688-L691)
- [DataPath.scala:49](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/datapath/DataPath.scala#L49)
- [DataPath.scala:666–691](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/datapath/DataPath.scala#L666-L691)
- [DataPath.scala:701–706](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/datapath/DataPath.scala#L701-L706)
- [IssueQueue.scala:935–961](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/issue/IssueQueue.scala#L935-L961)

PRF 배선 추가 근거: [Parameters.scala:342–446](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/Parameters.scala#L342-L446) · [RFReadArbiter.scala:194–236](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/datapath/RFReadArbiter.scala#L194-L236)

<a id="ref-bottleneck"></a>

### F. 정지·재선택·확장 검토

[본문 6절로 돌아가기](#q6)

여기서 12–16-wide 확장으로 연결한다. Dispatch 폭만 늘리면 IQ enqueue 한도, 특정 FU 후보 집중, PRF 공유 경쟁이 남는다. 실제 IPC와 임계 경로는 측정해야 하며 본편 예제로 수치를 추정하지 않는다.

**F.** 각 lane i까지의 IQ q 선택 수 C(i,q)를 누적한다. q가 ready이면 C(i,q)&gt;numEnq(q), ready가 아니면 C(i,q)≠0인 경우 해당 lane을 막는다. 이를 모든 IQ에 대해 OR하므로 앞선 lane이 막히면 뒤쪽도 기다릴 수 있다.

기준 정수 IQ의 numEnq는 2다. DIV, DIV, DIV, ADD가 들어오면 IQ1이 ready여도 세 번째 DIV부터 뒤쪽은 enqueue 한도를 넘는다. 본문 6절은 선택 IQ0의 ready가 0인 별도 사례다.

BalanceOpt가 켜져 있고 valid && !ready이면 현재 명령의 후보 선택 register를 다시 계산한다. 다음 cycle에 목적지가 달라질 수 있지만 같은 후보가 다시 선택될 수도 있다. 단일 후보 DIV는 IQ1에 남는다. 아직 IQ에 들어가지 않은 명령의 목적지 재선택과 IQ entry의 OG0 재발행은 다른 단계다.

**12–16-wide 검토.** FU별 후보 집중, IQ당 enqueue 폭, 고정 dequeue 서비스, PRF의 (port,bank) 공유, WB 예약을 함께 확인해야 한다. 측정 지표는 per-lane block 원인, IQ 분포, select 대비 OG0/OG1 성공률, FU 이용률이다. IPC·면적·전력·timing 수치는 본 문서에서 측정하지 않았다.

#### 소스 근거

- [IssueQueue.scala:223–224](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/issue/IssueQueue.scala#L223-L224)
- [IssueQueue.scala:1000–1005](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/issue/IssueQueue.scala#L1000-L1005)
- [IssueQueue.scala:1041–1059](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/issue/IssueQueue.scala#L1041-L1059)
- [EntryBundles.scala:238–240](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/issue/EntryBundles.scala#L238-L240)
- [EntryBundles.scala:575–576](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/issue/EntryBundles.scala#L575-L576)
- [Dispatch.scala:580–596](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/dispatch/Dispatch.scala#L580-L596)
- [Dispatch.scala:611–648](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/dispatch/Dispatch.scala#L611-L648)
- [Dispatch.scala:710–727](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/dispatch/Dispatch.scala#L710-L727)
- [Dispatch.scala:850–894](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/dispatch/Dispatch.scala#L850-L894)
- [Dispatch.scala:170–175](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/dispatch/Dispatch.scala#L170-L175)
- [Dispatch.scala:561–579](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/dispatch/Dispatch.scala#L561-L579)
- [Dispatch.scala:592–596](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/dispatch/Dispatch.scala#L592-L596)
- [CtrlBlock.scala:686–695](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/CtrlBlock.scala#L686-L695)
- [CtrlBlock.scala:766–781](https://github.com/OpenXiangShan/XiangShan/blob/f85a1f71da8c1522035744c71e2cb266bfed1a7d/src/main/scala/xiangshan/backend/CtrlBlock.scala#L766-L781)

### 자료의 범위

소스상의 클래스·생성 변수와 설정을 설명하며 합성 후 RTL instance path를 확정한 자료는 아니다. FP·Vector 전용 동작은 본문에서 다루지 않지만, 공유 ready·WB 조건이 제거된 하드웨어 구성을 가정하지 않는다.

[목차로 돌아가기](#contents)
