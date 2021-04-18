# Esk8.Board.Controller

## Cores
### Core 0
- display_task_0
  - send/read display queue
- batteryMeasureTask_0
### Core 1
- loop()
  - read_trigger
  - send_controller_packet_to_board
    - send_config if not connected
    - send packet otherwise
  - packetReceived()
    - send config if board_packet.id == 0
    - disp_queue if moving or stopped (on change)

--------------------------------------------------

### Tasks
- OrchestratorTask
  - Provides:
    - SendToBoardNotf
  - Consumes:
    - _N/A_
- BoardCommsTask
  - Provides:
    - PacketState
  - Consumes:
    - SendToBoardNotf
- Display
  - Provides:
    - _N/A_
  - Consumes:
    - SendToBoardNotf (maybe?)
    - PacketState
- NintendoClassicTask
  - Provides:
    - NintendoButtonEvent
  - Consumes:
    - SendToBoardNotf
    - PacketState
  - Special:
    - turns on/off when start/stop
- QwiicButtonTask
  - Provides:
    - PrimaryButtonState
  - Consumes:
    - SendToBoardNotf
- RemoteTask
  - Provides:
    - BatteryInfo
  - Consumes:
    - SendToBoardNotf
- StatsTask
  - Provides:
    - _N/A_
  - Consumes:
    - SendToBoardNotf
    - PacketState
  - Special:
    - turns on/off when start/stop



