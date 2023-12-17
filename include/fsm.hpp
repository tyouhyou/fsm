#pragma once

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#endif

#include <functional>
#include <tuple>
#include <map>
#include <stdint.h>
#include <stdexcept>
#include <mutex>
#include <memory>
#include "test.hpp"

namespace zb
{
    namespace fsm
    {

#if !defined(FSM_STATE_ID)
#define FSM_STATE_ID unsigned int
#endif

#if !defined(FSM_EVENT_ID)
#define FSM_EVENT_ID unsigned int
#endif

        enum class fsm_state_event_id : unsigned int
        {
            nothing = 0,
            entered,
            leaving,
            transit,
        };

        struct event_args
        {
            FSM_EVENT_ID id;
            // define other properties in inherited class/struct
        };

        struct state_event_args
        {
            fsm_state_event_id id;
            FSM_STATE_ID prev;
            FSM_STATE_ID next;
        };

        class state_machine;

        class state
        {
            friend class state_machine;

        public:
            inline FSM_STATE_ID &id() noexcept { return myid; }

            state &on(FSM_EVENT_ID &id, const std::function<void(const event_args &)> &action)
            {
                register_event_handler(id, action);
                return *this;
            }

            state &off(const FSM_EVENT_ID &id)
            {
                unregister_event_handler(id);
                return *this;
            }

            state &in(const std::function<void(const state_event_args &)> &fn)
            {
                register_state_event_handler(fsm_state_event_id::entered, fn);
                return *this;
            }

            state &out(const std::function<void(const state_event_args &)> &fn)
            {
                register_state_event_handler(fsm_state_event_id::leaving, fn);
                return *this;
            }

            state &transit_on(const FSM_EVENT_ID &id, const FSM_STATE_ID &next_state)
            {
                register_event_handler(id, [this, &next_state](const event_args &) -> void
                                       { this->transit(next_state); });
                return *this;
            }

        private:
            state() = delete;
            state(const FSM_STATE_ID &stid)
                : event_locker{}, myid{stid}
            {
            }

            FSM_STATE_ID myid;
            std::map<FSM_EVENT_ID, std::function<void(const event_args &)>> event_list;
            std::map<fsm_state_event_id, std::function<void(const state_event_args &)>> state_event_list;
            std::function<void(const state_event_args &)> request_transit;

            std::mutex event_locker;

            inline void set_transit_delegate(const std::function<void(const state_event_args &)> &fn)
            {
                std::lock_guard<std::mutex> lock(event_locker);
                request_transit = fn;
            }

            // when registered handler for same event id, the last one takes effective
            inline void register_state_event_handler(const fsm_state_event_id &id, const std::function<void(const state_event_args &)> &fn)
            {
                std::lock_guard<std::mutex> lock(event_locker);
                state_event_list[id] = fn;
            }

            // when registered handler for same event id, the last one takes effective
            void register_event_handler(const FSM_EVENT_ID &id, const std::function<void(const event_args &)> &fn)
            {
                std::lock_guard<std::mutex> lock(event_locker);
                event_list[id] = fn;
            }

            void unregister_event_handler(const FSM_EVENT_ID &id)
            {
                std::lock_guard<std::mutex> lock(event_locker);
                if (0 < event_list.count(id))
                {
                    event_list.erase(id);
                }
            }

            void transit(const FSM_STATE_ID &next_state)
            {
                std::lock_guard<std::mutex> lock(event_locker);
                if (!request_transit)
                    return;

                state_event_args args{fsm_state_event_id::transit, myid, next_state};
                request_transit(args);
            }

            void fire(const state_event_args &args)
            {
                std::lock_guard<std::mutex> lock(event_locker);
                if (0 >= state_event_list.count(args.id))
                {
                    return;
                }
                state_event_list[args.id](args);
            }

            void fire(const event_args &args)
            {
                std::lock_guard<std::mutex> lock(event_locker);
                if (0 >= event_list.count(args.id))
                {
                    return;
                }
                event_list[args.id](args);
            }
        };

        class state_machine
        {
        public:
            state_machine()
                : state_list{}, current_state{nullptr}
            {
            }
            ~state_machine() = default;

            state &state_(const FSM_STATE_ID &id)
            {
                auto st = get(id);
                if (nullptr != st)
                {
                    return *st;
                }

                struct _state : state
                {
                    _state(const FSM_STATE_ID &stid) : state(stid) {}
                };

                state_list[id] = std::make_shared<_state>(id);
                state_list[id]->set_transit_delegate([this](const state_event_args &args)
                                                     { this->transit(args); });
                return *state_list[id];
            }

            std::function<void(const event_args &)> listener()
            {
                return std::bind(&state_machine::event_listener, this, std::placeholders::_1);
            }

            std::shared_ptr<state> current() noexcept
            {
                return current_state;
            }

        private:
            std::shared_ptr<state> current_state;
            std::map<FSM_STATE_ID, std::shared_ptr<state>> state_list;

            void event_listener(const event_args &args)
            {
                if (current_state)
                {
                    current_state->fire(args);
                }
            }

            void transit(const state_event_args &args)
            {
                auto next = get(args.next);
                if (!next)
                    return;

                if (current_state)
                {
                    current_state->fire({fsm_state_event_id::leaving, args.prev, args.next});
                }
                current_state = next;
                current_state->fire({fsm_state_event_id::entered, args.prev, args.next});
            }

            std::shared_ptr<state> get(const FSM_STATE_ID &id) noexcept
            {
                std::shared_ptr<state> ret = nullptr;
                if (state_list.count(id))
                {
                    ret = state_list[id];
                }
                return ret;
            }
        };

    }
}

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif