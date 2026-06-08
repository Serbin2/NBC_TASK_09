# NumberBaseball (숫자 야구)

Unreal Engine 5.7 기반의 **멀티플레이 턴제 숫자 야구** 게임입니다.
플레이어들은 채팅으로 명령을 입력하여 게임을 시작하고, 자신의 차례에 세 자리 숫자를 추측해 정답을 맞춥니다.

서버 권위(server-authoritative) 모델로 설계되어, 모든 게임 판정과 상태 변경은 서버에서 처리되고
클라이언트는 검증된 상태를 복제(replication)받아 UI에 반영합니다.

---

## 게임 규칙

- 정답은 **서로 다른 세 자리 숫자**입니다. (리딩제로 허용, 예: `012`)
- 각 추측에 대해 판정 결과를 알려줍니다.
  - **스트라이크(S)**: 숫자와 자리가 모두 일치
  - **볼(B)**: 숫자는 있으나 자리가 다름
  - **아웃(Out)**: 일치하는 숫자가 하나도 없음 (`0S 0B`)
- **3 스트라이크**를 맞춘 플레이어가 승리하고 게임이 종료됩니다.
- 각 플레이어는 제한된 **도전 기회**(기본 3회)를 가집니다.
- 모든 플레이어가 기회를 소진하면 **무승부**로 종료됩니다.

---

## 턴제 진행

- 게임 시작 시점에 연결된 플레이어로 **턴 순서가 고정**됩니다.
- **첫 번째 차례는 무작위로** 선택됩니다.
- 자신의 차례인 플레이어만 도전할 수 있습니다.
- 각 턴에는 **제한시간(기본 120초)**이 있습니다.
  - 시간 내에 도전하지 않으면 **기회가 차감되고 다음 사람에게 차례가 넘어갑니다.**
- 차례를 넘길 때는 **남은 기회가 있는** 다음 플레이어를 찾아 넘깁니다. (기회가 없는 플레이어는 건너뜀)
- 게임 도중 플레이어가 **나가면 차례를 몰수**하고 턴 순서에서 제거합니다.
  - 나간 플레이어가 현재 차례였다면 즉시 다음 사람에게 넘깁니다.
  - 참가자가 모두 나가면 게임이 종료됩니다.
- 게임 도중 **새로운 플레이어는 참가할 수 없습니다.** (턴 순서에 포함되지 않아 도전 불가)

---

## 조작 방법 (채팅 명령)

채팅 입력창에 다음을 입력합니다.

| 입력 | 동작 |
| --- | --- |
| `/GameOn` | 게임을 시작합니다. (대소문자 무시) |
| `/123` | `/` 뒤에 세 자리 숫자로 도전합니다. (자신의 차례 + 유효한 숫자일 때만) |
| 그 외 텍스트 | 일반 채팅 메시지로 모두에게 전송됩니다. |

도전 입력은 **클라이언트에서 1차 검증**(게임 진행 여부 / 내 차례 / 숫자 형식 / 남은 기회)한 뒤,
서버에서 **다시 권위 검증**하여 처리합니다. 잘못된 입력은 본인에게만 사유가 표시됩니다.

---

## 아키텍처

서버 권위 모델로, 역할별로 클래스를 분리했습니다.

### `ANBGameModeBase` (서버 전용)
게임 진행의 **권위 주체**. 서버에만 존재합니다.
- 게임 시작(`RequestGameOn`), 추측 처리(`SubmitNumber`), 스트라이크/볼 판정(`JudgeGuess`)
- **턴 관리**: 턴 순서(`TurnOrder`) / 현재 차례(`CurrentTurnPC`) 보유, 차례 부여(`GiveTurnTo`) 및 전진(`TryAdvanceTurn` / `AdvanceTurnFrom`)
- **턴 제한시간 타이머**(`FTimerHandle`) 관리 및 타임아웃 처리(`OnTurnTimeout`)
- 플레이어 퇴장 처리(`Logout`)로 차례 몰수
- 게임 종료 정리(`EndGame` / `EndGameAsDraw`)

### `ANBGameState` (전체 복제)
게임 전반 상태의 **단일 진실 공급원**. 모든 클라이언트로 복제됩니다.
- `bIsGameOn` — 게임 진행 여부 (복제)
- `CorrectAnswer` — 정답 (서버에만 존재, **복제하지 않음** → 정답 노출 방지)
- `TurnEndServerTime` / `TurnDuration` — 턴 타이머 정보 (복제)
- `GetRemainingTurnRatio()` / `GetRemainingTurnSeconds()` — `GetServerWorldTimeSeconds()` 기반으로 서버/클라 모두에서 남은 시간 계산

### `ANBPlayerState` (전체 복제)
플레이어별 상태. 모든 클라이언트로 복제됩니다.
- `RemainChance` / `MaxChance` — 남은/최대 도전 기회 (복제)
- `bIsMyTurn` — 현재 내 차례인지 여부 (복제)
- `SpendChance()` / `SetMyTurn()` — 서버 권위 측에서만 변경 (`HasAuthority` 가드)

### `ANBPlayerController`
입력 처리와 클라이언트 측 검증을 담당합니다.
- `SendChatMessage` / `HandleCommand` — 채팅·명령 파싱
- `IsValidGuessNumber` — 클라이언트 측 숫자 유효성 검증
- `ServerSubmitNumber` / `ServerRequestGameOn` — 서버 RPC
- `ClientReceiveMessage` / `ClientReceiveSystemMessage` — 서버→클라 메시지 RPC
- `ShowLocalSystemMessage` — 본인에게만 보이는 로컬 안내 (네트워크 미사용)

### `UNBChatWidget` (UMG)
채팅 UI 및 턴 타이머 UI.
- 채팅 메시지 표시(`AddMessage`)와 입력 커밋 처리
- `NativeTick` → `UpdateTurnUI()`로 매 프레임 갱신:
  - `ProgressBar_TurnTimer` — 현재 턴 남은 시간 비율 표시
  - `Image_MyTurn` — 내 차례일 때만 표시(visibility 토글)

---

## 네트워킹 설계 노트

- **상태는 복제, 행동은 RPC**: 게임/플레이어 상태(`GameState`, `PlayerState`)는 복제로 전파하고,
  플레이어의 도전 입력은 `Server` RPC(`ServerSubmitNumber`)로 전달합니다.
- **정답 비노출**: `CorrectAnswer`는 복제 변수가 아니므로 클라이언트로 전송되지 않습니다.
- **턴 타이머는 "종료 시각"을 복제**: 남은 시간을 매 틱 복제하지 않고, 턴 종료 서버 시각(`TurnEndServerTime`)만
  턴당 1회 복제합니다. 클라이언트는 `GetServerWorldTimeSeconds()`(GameStateBase가 자동 동기화)로
  매 프레임 부드럽게 남은 시간을 계산합니다. → 대역폭 절약 + 부드러운 표시
- **이중 검증**: 클라이언트에서 사전 검증(UX)하고 서버에서 권위 재검증(보안)합니다.
- **현재 차례는 포인터로 추적**: `CurrentTurnPC`(약참조)로 추적하여, 퇴장으로 배열이 줄어도
  인덱스 보정 버그 없이 안전하게 차례를 넘깁니다.

---

## 게임 흐름

```
/GameOn 입력
   └─ 서버: GameState.bIsGameOn = true (복제), 정답 무작위 생성
            모든 참가자 InitGame(기회 초기화), TurnOrder 고정
            무작위 첫 차례 부여 → GiveTurnTo
                 └─ 턴 타이머(120초) 시작, GameState에 종료 시각 복제

/123 입력 (자신의 차례)
   ├─ 클라: 게임중 / 내차례 / 숫자유효 / 기회있음 검증 → ServerSubmitNumber
   └─ 서버: 권위 재검증 → SpendChance → JudgeGuess
             ├─ 3 스트라이크 → 승리, EndGame
             ├─ 그 외 → 결과+남은기회 브로드캐스트 → 다음 차례로 전진
             └─ 넘길 사람 없음 → 무승부, EndGame

제한시간 초과 (OnTurnTimeout)
   └─ 서버: SpendChance(차감) → 다음 차례로 전진 (없으면 무승부)

플레이어 퇴장 (Logout)
   └─ 서버: TurnOrder에서 제거(차례 몰수)
            현재 차례였다면 다음으로 전진 / 전원 퇴장 시 종료
```

---

