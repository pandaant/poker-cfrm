set style fill solid
set term png
set output "histogramm.png"
plot "plotdata" using 1:2 title 'AhKh' with boxes lc rgb "green"
