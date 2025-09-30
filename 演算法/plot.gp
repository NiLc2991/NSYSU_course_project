set terminal png size 800,800
unset key
set output 'fig.png'
set title 'dt5'
plot 'route.txt' with linespoints pointtype 7 linecolor 'blue'
