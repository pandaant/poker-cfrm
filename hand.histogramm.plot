set style fill solid
set term png
set mxtics 2
set output "histogramm.png"
plot "plotdata" using 1:2:xtic(1) with boxes
