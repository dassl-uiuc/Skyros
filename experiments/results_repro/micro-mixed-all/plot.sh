#! /bin/bash
#run this script to generate the lat vs thrpt graphs for c and w

#./plot_lat_thrpt.sh ../paper_results/wc-2 sync w 2;
#./plot_lat_thrpt.sh ../paper_results/wc-2 async w 2;
#./plot_lat_thrpt.sh ../paper_results/wc-2 sync c 2;

(cat <<EOF
	set terminal postscript eps enhanced color size 2.2, 1.8 font "Times-new-roman,22"
	set output "micro-mixed-all.eps"
	set xlabel "Write percentage (%)" font "Times-new-roman,22" offset 0,0.7,0
	set ylabel "Throughput (Kops/s)" font "Times-new-roman,22" offset 2.1,-0.2,0
	set tmargin 0.2
	set lmargin 5.5
	set rmargin 0.7
	set bmargin 3.6
	set yrange [0:135]
	set xrange [0:100]
	set tics scale 0.7
	set ytics 30
	set xtics 10,20,90 offset 0,0.2,0
	set key at 60,41 font "Times-new-roman,22" samplen 2.5
	set style function linespoints
	plot "output" using 1:2 title "Paxos" with linespoints dashtype 3 lw 4 lc rgb "red" pt 5 ps 1.25,\
		"output" using 1:3 title "Skyros" with linespoints dashtype 4 lw 4 lc rgb "blue" pt 1 ps 1.25

EOF
) | gnuplot -persist
