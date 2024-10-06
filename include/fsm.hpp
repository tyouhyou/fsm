#pragma once

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#endif

#include <functional>
#include <map>
#include <memory>
#include <vector>
#include <string>
#include <mutex>

// TODO: use std::invoke to replace template, which requires c++17 ?

namespace zb
{
    template <
        class Ty_state_id = int,
        class Ty_event_id = int,
        class Ty_event_arg = void*,
        class Ty_predict = std::function<bool(const Ty_event_arg&)>,
        class Ty_event_action = std::function<void(const Ty_event_arg&)>
    >
    class fsm_
    {
    private:
        struct transit_arg
        {
            Ty_state_id prev;
            Ty_state_id next;
            Ty_event_id evnt;
        };
        struct entity
        {
            Ty_predict predict;
            Ty_event_action act;
        };

        class state
        {
        private:

            class thenable
            {
            public:
                thenable& if_(const Ty_predict pred)
                {
                    _entity.predict = pred;
                    return *this;
                }
                state& then_(const Ty_event_action fun)
                {
                    _entity.act = fun;
                    return _state;
                }
                state& transit_(const Ty_state_id& state_id)
                {
                    _entity.act = [state_id](const Ty_event_arg& arg) {
                        transit_arg arg;
                        arg.prev = _state._id;
                        arg.next = state_id;
                        _state._fsm.transit_(arg);
                    };
                    return _state;
                }

            private:
                thenable(state& s, entity& eh)
                    : _state(s), _entity(eh)
                { /* do nothing */
                }

                state& _state;
                entity _entity;
                // TODO: make _entity to entities to container thenables. 
                // on(evt).if_().then_().elif_().then_().else_().then_();
                // at this time being: on(e1).if_().then_().on(e1).if_().then_() for same event.
            };

        public:
            state(const Ty_state_id& id, const fsm_& ref)
                : _id(id)
                , _fsm(ref)
                , _exit{}
                , _entry{}
                , _entities{}
            { /* do nothing */
            }

            ~state()
            {
                _thenable.reset();
                _thenable = nullptr;
                for (const auto& kvs : _entities)
                {
                    for (const auto& ent : kvs.second)
                    {
                        ent.second.act = nullptr;
                    }
                    kvs.second.clear();
                }
                _entities.clear();
            }

            state& in(const std::function<void(const Ty_event_id&, const Ty_state_id&)> handler)
            {
                _entry = handler;
                return *this;
            }

            thenable& out(const std::function<void(const Ty_event_id&, const Ty_state_id&> handler)
            {
                _exit = handler;
                return *this;
            }

            thenable& on(const Ty_event_id& id)
            {
                auto et = entity();
                return make_thenable(_entities[id]);
            }

            fsm_& end_config()
            {
                return _fsm;
            }

        private:
            const fsm_& _fsm;
            Ty_state_id _id;
            std::function<void(transit_arg arg)> _entry;
            std::function<void(Ty_state_id next)> _exit;
            std::shared_ptr<thenable> _thenable;
            std::map<Ty_state_id, std::vector<entity>> _entities;

            std::vector<entity> get_entity_list(const Ty_event_id& id)
            {
                if (_entities.end() == _entities.find(id))
                {
                    _entities[id] = std::vector<entity>();
                }
                return _entities[id];
            }

            entity& create_entity_to_list()
            {
                return get_entity_list(_id).emplace_back(entity());
            }

            thenable& make_thenable(entity& et)
            {
                if (_thenable) _thenable.reset();
                _thenable = std::make_shared<thenable>(*this, et);
                return *_thenable;
            }

            void handle(const Ty_event_id& id, const Ty_event_arg& arg)
            {
                if (_entities.end() == _entities.finded(id)) return;
                for (const auto& entity : _entities[id])
                {
                    if (entity.predict && !entity.predict(arg))
                    {
                        continue;
                    }
                    if (entity.act) entity.act(arg);
                    break;      // perform one callback only.
                }
            }
        };

    public:
        fsm_<Ty_state_id, Ty_event_id, Ty_event_arg, Ty_predict, Ty_event_action>() : _states{} {}

        virtual ~fsm_()
        {
            for (auto& state : _states)
            {
                state.second.reset();
            }
            _states.clear();
        }

        state& config(Ty_state_id event_id)
        {
            if (_states.find(event_id) == _states.end())
            {
                _states[event_id] = std::make_shared<state>(event_id, *this);
            }
            return *_states[event_id];
        }

        void run(const Ty_state_id& init_state)
        {
            _current_id = init_state;

            transit_arg transarg();
            transarg.prev = _current_id;
            transarg.next = _current_id;

            _current_state = _states[_current_id];
            _current_state->entry_(transarg);
        }

        void pub(const Ty_event_id& event_id, const Ty_event_arg& arg)
        {
            if (!_current_state) return;
            _current_state.handle(event_id, arg);
        }

        Ty_event_id current()
        {
            return _current_id;
        }

    private:
        Ty_state_id _current_id;
        std::shared_ptr<state> _current_state;
        std::map<Ty_event_id, std::shared_ptr<state>> _states;

        // when this private method is called, there must be a _current_state.
        void transit_to(const transit_arg& arg)
        {
            if (arg.next == _current_id) return;
            if (_states.find(arg.next) == _states.end()) return;

            if (_current_state && _current_state._exit)
                _current_state.exit_(arg);

            _current_id = arg.next;
            _current_state = _states[_current_id];

            if (_current_state._entry)
                _current_state._entry(arg);
        }
    };

    using fsm_def = fsm_<>;
}

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
