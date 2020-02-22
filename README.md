# Esk8.Board.Controller

## Cores
### Core 0
- EncoderTask
  - xQueueRx: xDeadmanChangedQueue
    - toggles encoder max
### Core 1
- loop()
  - read_trigger
  - send_controller_packet_to_board
    - send_config if not connected
  - packetReceived()
