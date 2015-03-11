set term png medium size 400, 320
set logscale xy
set xlabel "Milliseconds"
set ylabel "Exploitability"
set title "No-Limit Leduc"
set xrange[1000:]

set output "leduc_time_sampler.png"
plot "leduc.time.data" using 1:2 title 'Chance Sampling' with lines,\
         "leduc.time.data" using 3:4 title 'External Sampling' with lines,\
         "leduc.time.data" using 5:6 title 'Outcome Sampling' with lines
