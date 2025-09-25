#pragma once
#include <stdexcept>

struct State {
  virtual ~State() = default;
  virtual void set_up() { throw std::logic_error("set_up() not implemented"); };
  virtual void set_up(int) {
    throw std::logic_error("set_up(int) not implemented");
  };

  virtual void pay_off() {};
};

struct NormalState : public State {

  void set_up() override;
  void pay_off() override;
};

struct CombatState : public State {

  void set_up() override;
  void pay_off() override;
};

struct TransitState : public State {
  int some_data;

  TransitState(int s) : some_data(s) {}

  void set_up(int something) override;
  void pay_off() override;
};
