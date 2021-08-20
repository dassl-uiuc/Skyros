#! /bin/bash
#run this script to generate the lat vs thrpt graphs for c and w

#./plot_lat_thrpt.sh ../paper_results/wc-2 sync w 2;
#./plot_lat_thrpt.sh ../paper_results/wc-2 async w 2;
#./plot_lat_thrpt.sh ../paper_results/wc-2 sync c 2;

(cat <<EOF
	set terminal postscript eps enhanced color size 2, 1.6 font "Times-new-roman,22"
	set output "ycsb-b-read.eps"
	set multiplot
	set xlabel "Latency (us)" font "Times-new-roman,22" offset 0,0.4,0
	set ylabel "CDF" font "Times-new-roman,22" offset 2.4,0.2,0
	set tmargin 0.5
	set lmargin 5
	set rmargin 0.75
	set bmargin 2.7
	set yrange [0:100]
	set tics scale 0.7 nomirror
	set xrange [0:630]
	set xtics 150
	#set xtics offset -0.5,0,0
	#set logscale x
	set border 3
	#set format x "10^{%L}"
	unset key
	set key at 455,31 font "Times-new-roman,22" samplen 2.5
	set style function linespoints
	
	#set arrow from 0,99 to 600,99 nohead lt 0 lw 2 lc rgb "#696969" 
	#set arrow from 0,100 to 600,100 nohead lt 0 lw 2 lc rgb "#696969"
	#set arrow from 0,99 to 0,100 nohead lt 0 lw 2 lc rgb "#696969" 
	#set arrow from 600,99 to 600,100 nohead lt 0 lw 2 lc rgb "#696969"
	#set arrow from 300,99 to 200,87  nohead lt 0 lw 2 lc rgb "#696969"

	set arrow from 200,87 to 580,87 nohead lt 1 lw 1 lc rgb "#696969"
	set arrow from 200,31 to 580,31 nohead lt 1 lw 1 lc rgb "#696969" 

	set arrow from 200,87 to 200,31 nohead lt 1 lw 1 lc rgb "#696969"
	set arrow from 580,87 to 580,31 nohead lt 1 lw 1 lc rgb "#696969" 

	set label 'close-up' at 300,94 font "Times-new-roman,19" front 

	plot "b_orig_read" using 2:1 title "Paxos" with lines dashtype 3 lw 4 lc rgb "red",\
		"b_rtop_read" using 2:1 title "Skyros" with lines dashtype 4 lw 4 lc rgb "blue" 


	unset xlabel
	unset ylabel
	unset object
	unset label
	unset tics
	unset arrow
	set border lw 0.5
	set yrange [99:100]
	set xrange [0:600]
	set xtics 300 font "Times-new-roman,19" offset 0,0.2 nomirror
	set ytics 99.1,.4,99.9 font "Times-new-roman,19" offset 0.65,0 nomirror
	set origin 0.38,0.34	
	set size 0.5,0.55
	unset format x
	set tics scale 0.5
	#set arrow from 100,99.95 to 300,99.95 nohead lt 0 lw 3
	#set arrow from 100,99.65 to 300,99.65 nohead lt 0 lw 3
	#set label '0.3%' at 310,99.8 font "Times-new-roman,20" front 
	#set arrow heads filled from 200,99.95 to 200,99.65 lt 2 lw 2 lc rgb "black" front
	#set label '5%' at 475, 97 font "Times-new-roman,20" front 

	unset key

	#set arrow heads filled from 450,93.9 to 450,99.9 lt 0 lw 5 front


	plot "b_orig_read" using 2:1 title "Paxos" with lines dashtype 3 lw 4 lc rgb "red",\
		"b_rtop_read" using 2:1 title "Skyros" with lines dashtype 4 lw 4 lc rgb "blue" 


	unset multiplot

EOF
) | gnuplot -persist
