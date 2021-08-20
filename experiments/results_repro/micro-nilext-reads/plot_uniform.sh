#! /bin/bash
#run this script to generate the lat vs thrpt graphs for c and w

#./plot_lat_thrpt.sh ../paper_results/wc-2 sync w 2;
#./plot_lat_thrpt.sh ../paper_results/wc-2 async w 2;
#./plot_lat_thrpt.sh ../paper_results/wc-2 sync c 2;

(cat <<EOF
	set terminal postscript eps enhanced color size 2, 1.8 font "Times-new-roman,22"
	set output "nilext_read_uniform.eps"
	set xlabel "Write percentage (%)" font "Times-new-roman,22" offset 0,0.7,0
	set label "uniform" font "Times Italic,23" at 32,870
	set ylabel "Latency (us)" font "Times-new-roman,22" offset 2.5,-0.2,0
	set tmargin 2
	set lmargin 6
	set rmargin 0.2
	set bmargin 3.6
	set ytics 250
	set yrange [0:1000]
	set xrange [0:100]
	set tics scale 0.7
	set xtics 10,20,90 offset 0,0.2,0
	set key at 106,1340 font "Times-new-roman,22" samplen 2.5
	set style function linespoints
	plot "avg-uniform" using 1:4 title "Paxos (mean)" with linespoints dashtype 3 lw 4 lc rgb "red" pt 5 ps 1.25,\
		"avg-uniform" using 1:6 title "" with linespoints dashtype 4 lw 4 lc rgb "blue" pt 1 ps 1.3,\
		"99.0-uniform" using 1:4 title "Paxos (p99)" with linespoints dashtype 3 lw 4 lc rgb "red" pt 8 ps 1.25,\
		"99.0-uniform" using 1:6 title "" with linespoints dashtype 4 lw 4 lc rgb "blue" pt 13 ps 1.4

EOF
) | gnuplot -persist
