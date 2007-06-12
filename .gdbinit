#file tzx2pzx
#file csw2pzx
file pzx2wav
break main
#break csw_render
break wav_out
break wav_close
#run <data3/StarBike2.csw
run <data/Chronos.pzx -o data/Chronos.wav
