

### Times
 - DebounceDelay: 50ms
 - LongPressDelay: 750ms
 - DoubleClickWindow: 500ms

### Events
 - Idle
 - Down: Button down.
 - Up: Button up.

Between each event there is a `DebounceDelay` pause where no other events can be triggered to debounce the switch. During this period no state transitions may happen and the `None` action is returned.

### Actions
 - None
 - Click
 - DoubleClick
 - LongPress
 - Release

### States
```mermaid
stateDiagram
    Start --> Pressed: Down
	Pressed --> Click: Up
	Click --> DoubleClick: Down
	Click --> Release: DoubleClickWindow
	DoubleClick --> Release: Up
	Pressed --> LongPress: LongPressDelay
	LongPress --> Release: Up
	Release --> Start
```

 The states `Click`, `DoubleClick`, and `Release` each have an associated `Action` which is returned when there is a transition to the matching `State`. Each `Action` should only be emitted in the first transition to a state.

### Result
```C
typedef struct button_result_t {
	Action action;
	uint32_t start;
	uint32_t end;
	Event event;
} ButtonResult;
```

When action is not `None`, `start` is the time of the `Down` event that transitioned to `Paused` and `end` is the time that the the transition to the `Action` state occurred.

Whenever there is a transition in the button pin value and it is more then a `DebounceDelay` duration from the last event the event is set to `Up` if the pin is low, or `Down` if the pin is high. If a transitions dose not occur the `Idle` event is returned.