#include "fsm.hpp"
#include "test.hpp"

using namespace zb;

int main(void) 
{
  fsm_<fsm_state_id> fsm;
  fsm.config(fsm_state_id::init)
      .in([](const fsm_::transit_arg &arg) {})
      .out([](const fsm_::transit_arg &arg) {})
      .on(FSM_EVENT_ID::timeout).then_([](const void* arg){})
      .end_config()
      ;

  return 0;
}

template zb::fsm_<fsm_state_id>;
