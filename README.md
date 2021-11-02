# ACServoMotionSimulator



## Motor Driver Settings

1. Motor Working   
    - pn003 = 0 : automatically enable drive after power on   
    - pn033 = 1 : origin regression trigger by GOH   
    - pn034 = 4 : origin regression reference point mode (4 or 5)   
    - pn035 = 2 : origin regression origin mode   
    - pn076 = 1 : emergency recovery   
    - Pn117 = 1 : internal position instructions   
    
2. Parameters   
    - pn109 = 1 : one time smooth filtering for acc. and dec.   
    - pn110 = 50 : smoothing filter time   
    - pn113 = 100 : feedforward %   
    - pn114 = 50 : feedforward filter time   
    
3. Communication   
    - pn065 = n : communication station number   
    - pn068 = 67 : control emergency, alarm reset, and servo on by communication   
    - pn069 = 32767 : control all functions by communication   
    - pn070 = 32703   
    - pn071 = 32767   
    
