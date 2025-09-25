#include "state.hpp"

void NormalState::set_up() {}
void NormalState::pay_off() {}

void CombatState::set_up() {}
void CombatState::pay_off() {}

void TransitState::set_up(int something) { some_data = something; };
void TransitState::pay_off() {};
