#! /bin/bash
#run this script to generate the lat vs thrpt graphs for c and w

#./plot_lat_thrpt.sh ../paper_results/wc-2 sync w 2;
#./plot_lat_thrpt.sh ../paper_results/wc-2 async w 2;
#./plot_lat_thrpt.sh ../paper_results/wc-2 sync c 2;

(cat <<EOF
	set terminal postscript eps enhanced color size 2, 1.8 font "Times-new-roman,22"
	set output "micro-nilext-only.eps"
	set xlabel "Throughput (Kops/s)" font "Times-new-roman,23" offset 0,0.5,0
	set ylabel "Latency (us)" font "Times-new-roman,23" offset 1.5,0,0
	set tmargin 3.1
	set lmargin 5.8
	set rmargin 0.3
	set bmargin 2.7
	set yrange [0:900]
	set ytics 200
	set xrange [0:140]
	set xtics 50
	set tics scale 0.7
	unset key
	set key spacing 0.85

	set key at 150,1350 font "Times-new-roman, 22" samplen 3
	set style function linespoints
	plot "w-orignobatch" using 2:3 title "Paxos (no batch)" with linespoints dashtype 2 lw 5 lc rgb "black" pt 2 ps 1.45,\
	"w-orig" using 2:3 title "Paxos" with linespoints dashtype 3 lw 5 lc rgb "red" pt 5 ps 1.4,\
		"w-rtop" using 2:3 title "Skyros" with linespoints dashtype 4 lw 5 lc rgb "blue" pt 1 ps 1.7
		


EOF
) | gnuplot -persist
