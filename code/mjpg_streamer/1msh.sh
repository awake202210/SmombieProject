
export STREAMER_PATH=$HOME/mjpg-streamer/mjpg-streamer-experimental
export LD_LIBRARY_PATH=$STREAMER_PATH
$STREAMER_PATH/mjpg_streamer -i "input_file.so -f /stream -n image01.jpg -d 0" -o "output_http.so -w $STREAMER_PATH/www"
