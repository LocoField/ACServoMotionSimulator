# ACServoMotionSimulator



## Motor Driver Settings

1. Motor Working   
    - pn003 = 1 : automatically enable drive after power on   
    - pn033 = 3 : origin regression at power on   
    - pn034 = 5 : origin regression reference   
    - pn035 = 2 : origin regression origin   
    - pn076 = 1 : emergency recovery   
    - Pn117 = 1 : internal position instructions   
    
2. Parameters   
    - pn109 = 1 : one time smooth filtering for acc. and dec.   
    - pn110 = 50 : smoothing filter time   
    - pn113 = 20 : feedforward %   
    - pn114 = 10 : feedforward filter time   
    
3. Communication   
    - pn065 = n : communication station number   
    - pn068 = 64 : control emergency only by communication   
    - pn069 = 32767 : control all functions by communication   
    - pn070 = 32690   
    - pn071 = 32767   
    
