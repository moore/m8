# m8

### Button Use
```mermaid
stateDiagram
    direction LR
    Sleep --> Idle: Single
    Idle --> Conversation: Double
    Idle --> Stimulate: Long
    Idle --> Idle: Single
    Conversation --> Conversation: Single
    Stimulate --> Stimulate: Single
    Timeout --> Sleep
```

### Happiness
```mermaid
stateDiagram
    Idle --> Ignore: Decrease
    Sleep --> Ignore: Decrease
    Sleep --> Idle: Increase
    Idle --> Game
    Game --> Idle: Increase

```

### Pet Game
```mermaid
stateDiagram
    Idle --> Pet: Click
    Pet --> Idle
    OverStimulated: Over Stimulated
    Idle --> OverStimulated: Click
    OverStimulated --> Idle: Delay Function

```

### Conversation F
Repeats three times.
```mermaid
stateDiagram
    direction LR
    [*] --> Pattern
    Pattern --> Response: Clicks
    Response --> Pattern
```
### Conversation M
Repeats three times.
```mermaid
stateDiagram
    direction LR
    [*] --> Pattern
    Pattern --> Interruption: Click
    Interruption --> Mad
    Mad --> Pattern
    Pattern --> Acknowledge: Click
    Acknowledge --> Pattern
    Pattern --> Ignore
    Ignore --> Mad
```

### Stimulate F
```mermaid
stateDiagram
    direction LR
    [*] --> Show
    Show --> Delay
    Delay --> Stimulate: Click
    Stimulate --> Show
    Stimulate --> Rainbow: Satisfied
```

### Stimulate M
```mermaid
stateDiagram
    direction LR
    [*] --> Ready
    Ready --> Ready: Click
    Ready --> Rainbow: After 15 Clicks
```