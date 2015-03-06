set term png medium size 400, 320
set logscale xy

set output "leduc_sampler.png"
plot "leduc.data" using 1:2 title 'Chance Sampling' with lines,\
         "leduc.data" using 1:3 title 'External Sampling' with lines,\
         "leduc.data" using 1:4 title 'Outcome Sampling' with lines
