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
- BoardCommsTask
  - Consumes:
    - SendToBoardNotf
      - sends packet to board
  - loops on own cadence (50ms?)
  - on update: sends PacketState to queue
- Display
  - Provides:
    - DisplayEvent (selecting certain properties)
      - property id
      - property value (integer?)
  - Consumes:
    - PacketState (for online/offline)
    - PrimaryButtonState
    - NintendoClassic
    - ThrottleState
- NintendoClassicTask
  - Provides:
    - NintendoButtonEvent
  - Consumes:
    - SendToBoardNotf (response)
    - PacketState (maybe enable/disable?)
  - Special:
    - turns on/off when start/stop
- QwiicButtonTask
  - Provides:
    - PrimaryButtonState
  - Consumes:
    - SendToBoardNotf (response)
- RemoteTask
  - Provides:
    - BatteryInfo (not very often, maybe when stopped?)
  - Consumes:
    - SendToBoardNotf
    - PacketState (start/stop)
- StatsTask
  - Provides:
    - _N/A_
  - Consumes:
    - SendToBoardNotf
    - PacketState (start/stop)
  - Special:
    - turns on/off when start/stop



