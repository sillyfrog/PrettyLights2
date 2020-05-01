#!/bin/sh -x
STARTING='#2b2b2b'
OUTER_ON='#888888'
INNER_ON='#ffffff'
BLACK='#000000'
# Initial fades on and off
./chaserlights.py 120 --off $STARTING --on $OUTER_ON --fadeon 10 --ontime 0 --fadeoff=10  --waittime 10    --offset 0 outer_fades.png 
# Final fade to complete off
./chaserlights.py 20 --off $BLACK --on $OUTER_ON --fadeon 0 --ontime 0 --fadeoff=10  --waittime 10    --offset 0 outer_fades_end.png 

./chaserlights.py 120 --off $STARTING --on $INNER_ON --fadeon 10 --ontime 0 --fadeoff=10  --waittime 10    --offset 0 inner_fades.png 
./chaserlights.py 20 --off $BLACK --on $INNER_ON --fadeon 0 --ontime 0 --fadeoff=10  --waittime 10    --offset 0 inner_fades_end.png 

# Now smash the above together to get the inner and outer fades (only)
./repeatpattern.py 120 outer_fades_end.png outer_fades.png --offset 100 --repeatcount 1 --pattern 0 --color '#00000000'
./repeatpattern.py 120 inner_fades_end.png inner_fades.png --offset 100 --repeatcount 1 --pattern 0 --color '#00000000'

# Put these onto the super laser master image, offsetting for each of the outter/inner stages
./repeatpattern.py 375 outer_fades.png SuperLaser.png --offset 96  --repeatcount 1 --pattern 0 --color '#00000000'
./repeatpattern.py 375 outer_fades.png SuperLaser.png --offset 101 --repeatcount 1 --pattern 1 --color '#00000000'
./repeatpattern.py 375 outer_fades.png SuperLaser.png --offset 96  --repeatcount 1 --pattern 2 --color '#00000000'
./repeatpattern.py 375 outer_fades.png SuperLaser.png --offset 101 --repeatcount 1 --pattern 3 --color '#00000000'
./repeatpattern.py 375 outer_fades.png SuperLaser.png --offset 96  --repeatcount 1 --pattern 4 --color '#00000000'
./repeatpattern.py 375 outer_fades.png SuperLaser.png --offset 101 --repeatcount 1 --pattern 5 --color '#00000000'
./repeatpattern.py 375 outer_fades.png SuperLaser.png --offset 96  --repeatcount 1 --pattern 6 --color '#00000000'
./repeatpattern.py 375 outer_fades.png SuperLaser.png --offset 101 --repeatcount 1 --pattern 7 --color '#00000000'
./repeatpattern.py 375 outer_fades.png SuperLaser.png --offset 96  --repeatcount 1 --pattern 8 --color '#00000000'
./repeatpattern.py 375 outer_fades.png SuperLaser.png --offset 101 --repeatcount 1 --pattern 9 --color '#00000000'
./repeatpattern.py 375 outer_fades.png SuperLaser.png --offset 96  --repeatcount 1 --pattern 10 --color '#00000000'
./repeatpattern.py 375 outer_fades.png SuperLaser.png --offset 101 --repeatcount 1 --pattern 11 --color '#00000000'
./repeatpattern.py 375 outer_fades.png SuperLaser.png --offset 96  --repeatcount 1 --pattern 12 --color '#00000000'
./repeatpattern.py 375 outer_fades.png SuperLaser.png --offset 101 --repeatcount 1 --pattern 13 --color '#00000000'
./repeatpattern.py 375 outer_fades.png SuperLaser.png --offset 96  --repeatcount 1 --pattern 14 --color '#00000000'
./repeatpattern.py 375 outer_fades.png SuperLaser.png --offset 101 --repeatcount 1 --pattern 15 --color '#00000000'
# Inners
./repeatpattern.py 375 inner_fades.png SuperLaser.png --offset 96 --repeatcount 1 --pattern 16 --color '#00000000'
./repeatpattern.py 375 inner_fades.png SuperLaser.png --offset 101 --repeatcount 1 --pattern 17 --color '#00000000'
./repeatpattern.py 375 inner_fades.png SuperLaser.png --offset 106 --repeatcount 1 --pattern 18 --color '#00000000'
./repeatpattern.py 375 inner_fades.png SuperLaser.png --offset 111 --repeatcount 1 --pattern 19 --color '#00000000'
./repeatpattern.py 375 inner_fades.png SuperLaser.png --offset 116 --repeatcount 1 --pattern 20 --color '#00000000'

# Finally, repeat that onto the full pattern backup
./repeatpattern.py 1500 SuperLaser.png Death_Star_All_Patterns_pwm.png --pattern 4 --waittime 0