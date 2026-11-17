#include "StateMachine.h"

StateMachine::StateMachine() { currentState = nullptr; }

StateMachine::~StateMachine() {
    delete pendingState;
    if (currentState) {
        currentState->onExit();
        delete currentState;
    }
}

void StateMachine::begin(State* initialState) {
    ChangeState(initialState);
    applyPendingState();
}

void StateMachine::update() {
    if (currentState) {
        clocks.tiempo_actual = millis();
        currentState->execute();
        applyPendingState();
    }
}

void StateMachine::ChangeState(State* newState) {
    if (!newState || newState == currentState || newState == pendingState) return;
    delete pendingState;
    pendingState = newState;
}

void StateMachine::applyPendingState() {
    if (pendingState) {
        State* newState = pendingState;
        pendingState = nullptr;
        Serial.print("Transición: ");
        Serial.print(currentState ? currentState->getName() : "NULL");
        Serial.print(" -> ");
        Serial.println(newState->getName());

        if (currentState) currentState->onExit();
        delete currentState;
        currentState = newState;
        currentState->setStateMachine(this);
        currentState->onEnter();
    }
}

const char* StateMachine::getCurrentStateName() {
    return currentState ? currentState->getName() : "NULL";
}
