#pragma once

#define FSM_STATE_ID fsm_state_id
#define FSM_EVENT_ID fsm_event_id

enum class fsm_state_id : unsigned int {
  chaos = 0,
  init,
  running,
  end,
};

enum class fsm_event_id : unsigned int {
  inited,
  timeout,
  ended,
};
