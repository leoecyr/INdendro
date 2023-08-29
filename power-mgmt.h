

#define BAT_LVL_PIN A2

void deep_sleep(int seconds) {
#if defined DEBUG
    Serial.print("Sleeping for ");
    Serial.println(TX_DELAY_SEC);
#endif

  // A low budget long, low-power sleep
  int tx_delay_count = round(seconds / 8);

  // count of 8s sleeps -- 225 = 1800s
  for(int slp_cnt = 0; slp_cnt < tx_delay_count; slp_cnt++) {
    // Enter power down state for 8 s with ADC and BOD module disabled
    LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);      
  }

#if defined DEBUG
    Serial.println("Waking up");
#endif

  // After sleeping for seconds, return to main loop
  return;
}
