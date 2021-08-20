#! /bin/bash
#run this script to generate the lat vs thrpt graphs for c and w

#./plot_lat_thrpt.sh ../paper_results/wc-2 sync w 2;
#./plot_lat_thrpt.sh ../paper_results/wc-2 async w 2;
#./plot_lat_thrpt.sh ../paper_results/wc-2 sync c 2;

(cat <<EOF
	set terminal postscript eps enhanced color size 2.15, 1.8 font "Times-new-roman,22"
	set output "exp3.eps"
	set xlabel "% reads to window" font "Times-new-roman,22" offset 0,0.5,0
	set ylabel "Latency (us)" font "Times-new-roman,22" offset 2,0,0
	set tmargin 0.25
	set lmargin 5.5
	set rmargin 1.1
	set bmargin 2.7
	set yrange [0:210]
	#set xrange [0:100]
	set logscale x
	set tics scale 0.7
	set xtics offset -0.4,0,0
	set key spacing 0.85
	set key at 120,90  font "Times-new-roman,21 samplen 1.7
	#unset key
	set style function linespoints
	plot "orig" using 1:2 title "Paxos" with linespoints dashtype 3 lw 4 lc rgb "red" pt 5 ps 1.25,\
		"100" using 1:2 title "Skyros [0-100us]" with linespoints dashtype 4 lw 4 lc rgb "blue" pt 4 ps 1.25,\
		"200" using 1:2 title "Skyros [0-200us]" with linespoints dashtype 4 lw 4 lc rgb "blue" pt 7 ps 1.25,\
		"1000" using 1:2 title "Skyros [0-1ms]" with linespoints dashtype 4 lw 4 lc rgb "blue" pt 1 ps 1.25
		

EOF
) | gnuplot -persist
