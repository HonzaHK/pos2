SOURCE="p2.c"
EXECUTABLE="p2"
FLAGS="-std=gnu99 -Wall -Wextra -pedantic -pthread"

rm $EXECUTABLE
gcc $FLAGS $SOURCE -o $EXECUTABLE

./$EXECUTABLE $1 $2