#! /bin/bash
#run this script to generate the lat vs thrpt graphs for c and w

#./plot_lat_thrpt.sh ../paper_results/wc-2 sync w 2;
#./plot_lat_thrpt.sh ../paper_results/wc-2 async w 2;
#./plot_lat_thrpt.sh ../paper_results/wc-2 sync c 2;

(cat <<EOF
	set terminal postscript eps enhanced color size 2, 1.6 font "Times-new-roman,22"
	set output "kvcurp-write.eps"
	set multiplot
	set xlabel "Latency (us)" font "Times-new-roman,22" offset 0,0.4,0
	set ylabel "CDF" font "Times-new-roman,22" offset 2.4,0.2,0
	set tmargin 0.5
	set lmargin 5
	set rmargin 0.75
	set bmargin 2.7
	set yrange [80:100]
	set tics scale 0.7 nomirror
	set xrange [0:1000]
	set xtics 300
	#set xtics offset -0.5,0,0
	#set logscale x
	set border 3
	#set format x "10^{%L}"
	unset key
	set key at 1000,91 font "Times-new-roman,22" samplen 2.3
	set style function linespoints

	plot "orig_write" using 2:1 title "Paxos" with lines dashtype 3 lw 4 lc rgb "red",\
		"rtop_write" using 2:1 title "Skyros" with lines dashtype 4 lw 4 lc rgb "blue",\
		"curp_write" using 2:1 title "Curp-c" with lines dashtype 1 lw 4 lc rgb "green" 

	unset multiplot

EOF
) | gnuplot -persist
