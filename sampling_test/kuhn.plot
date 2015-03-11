set term png medium size 400, 320
set logscale xy
set xlabel "Iterations"
set ylabel "Exploitability"
set title "No-Limit Kuhn"

set output "kuhn_sampler.png"
plot "kuhn.data" using 1:2 title 'Chance Sampling' with lines,\
         "kuhn.data" using 1:3 title 'External Sampling' with lines,\
         "kuhn.data" using 1:4 title 'Outcome Sampling' with lines

