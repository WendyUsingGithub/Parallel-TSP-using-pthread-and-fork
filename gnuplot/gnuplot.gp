# gnuplot gnuplot.gp
set style line 1 \
    linecolor rgb '#000000' \
    linetype 1 linewidth 4 \
    pointtype 7 pointsize 1

set terminal svg size 1800 , 1200
set output "outputData.svg"
unset key
plot 'outputData.dat' with linespoints linestyle 1