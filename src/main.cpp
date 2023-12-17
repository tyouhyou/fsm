#include "fsm.hpp"

using namespace zb::fsm;

int main(void) {
  state_machine fsm;
  fsm.state_(FSM_STATE_ID::init)
      .in([](const state_event_args &arg) {})
      .out([](const state_event_args &arg) {})
      .on(FSM_EVENT_ID::timeout, [](const event_args &arg) {});

  return 0;
}