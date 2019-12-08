# Esk8.Board.Controller

##Cores
###Core 0
- EncoderTask
  - xQueueRx: xDeadmanChangedQueue
    - toggles encoder max
###Core 1
- StateMachineTask
  - xQueueRx: xStateMachineQueue
  - fsm.run_machine()
- loop()
  - Scheduler
    - t_SendToBoard() (xSendPacketToBoardQueue)
  - packetReceived()
