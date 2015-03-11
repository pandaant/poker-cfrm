set term png medium size 400, 320
set logscale xy
set xlabel "Milliseconds"
set ylabel "Exploitability"
set xrange[10:]
set title "No-Limit Kuhn"

set output "kuhn_time_sampler.png"
plot "kuhn.time.data" using 1:2 title 'Chance Sampling' with lines,\
         "kuhn.time.data" using 3:4 title 'External Sampling' with lines,\
         "kuhn.time.data" using 5:6 title 'Outcome Sampling' with lines

