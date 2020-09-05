sudo docker run --name tpe -v ${PWD}:/home --security-opt seccomp:unconfined -ti agodio/itba-so:1.0 
# sudo docker exec -it tpe /bin/bash