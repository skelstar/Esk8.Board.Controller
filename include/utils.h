

bool idFromBoardExpected(long id) {
  return lastIdFromBoard == NO_PACKET_RECEIVED_FROM_BOARD 
      || id == lastIdFromBoard + 1;
}

void updateCanAccelerate(bool newState) {
  Serial.printf("canAccelerate changed: %d\n", newState);
  canAccelerate = newState;
}

bool getCanAccelerateCallback() {
  return canAccelerate;
}
