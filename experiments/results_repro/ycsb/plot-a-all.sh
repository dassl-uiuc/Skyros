#! /bin/bash
#run this script to generate the lat vs thrpt graphs for c and w

#./plot_lat_thrpt.sh ../paper_results/wc-2 sync w 2;
#./plot_lat_thrpt.sh ../paper_results/wc-2 async w 2;
#./plot_lat_thrpt.sh ../paper_results/wc-2 sync c 2;

(cat <<EOF
	set terminal postscript eps enhanced color size 2, 1.6 font "Times-new-roman,22"
	set output "ycsb-a-all.eps"
	set multiplot
	set xlabel "Latency (us)" font "Times-new-roman,22" offset 0,0.4,0
	set ylabel "CDF" font "Times-new-roman,22" offset 2.4,0.2,0
	set tmargin 0.5
	set lmargin 5
	set rmargin 0.75
	set bmargin 2.7
	set yrange [0:100]
	set tics scale 0.7 nomirror
	set xrange [0:1000]
	set xtics 300
	set border 3
	set style function linespoints
	#set label '\~2x' at 110,80 font "Times-new-roman,19" front 
	#set arrow heads filled from 102.675,70 to 201.5,70 lt 0 lw 2 front size screen 0.017,13
	
	#set label 'reads' at 150, 30 font "Times-new-roman,19" 
	#set label '(1rtt)' at 150, 20 font "Times-new-roman,19" 
	#set arrow heads filled from 125,0 to 125, 50 lt 0 lw 2 front size screen 0.02,15 
	#set label 'writes' at 300, 80 font "Times-new-roman,19" 
	#set label '(2rtt)' at 300, 70 font "Times-new-roman,19" 
	#set arrow heads filled from 275,50 to 275, 100 lt 0 lw 2 front size screen 0.02,15

	set key at 700,31 font "Times-new-roman,22" samplen 2.5
	set style function linespoints
	#set arrow from 0,95 to 800,95 nohead lt 0 lw 2 lc rgb "#696969" 
	#set arrow from 0,100 to 800,100 nohead lt 0 lw 2 lc rgb "#696969"
	#set arrow from 0,95 to 0,100 nohead lt 0 lw 2 lc rgb "#696969" 
	#set arrow from 800,95 to 800,100 nohead lt 0 lw 2 lc rgb "#696969"
	#set arrow from 400,99 to 270,87  nohead lt 0 lw 2 lc rgb "#696969"

	set arrow from 270,87 to 960,87 nohead lt 1 lw 1 lc rgb "#696969"
	set arrow from 270,31 to 960,31 nohead lt 1 lw 1 lc rgb "#696969" 

	set arrow from 270,87 to 270,31 nohead lt 1 lw 1 lc rgb "#696969"
	set arrow from 960,87 to 960,31 nohead lt 1 lw 1 lc rgb "#696969" 

	set label 'close-up' at 450,94 font "Times-new-roman,19" front 


	plot "a_orig" using 2:1 title "Paxos" with lines dashtype 3 lw 4 lc rgb "red",\
		"a_rtop" using 2:1 title "Skyros" with lines dashtype 4 lw 4 lc rgb "blue" 
	
	unset xlabel
	unset ylabel
	unset object
	unset label
	unset tics
	unset arrow
	set border lw 0.5
	set yrange [95:100]
	set xrange [0:800]
	set xtics 300 font "Times-new-roman,19" offset 0,0.2 nomirror
	set ytics 95,2,99.9 font "Times-new-roman,19" offset 0.65,0 nomirror
	set origin 0.3,0.34	
	set size 0.63,0.55
	unset format x
	set tics scale 0.5
	#set arrow heads filled from 365,99 to 215,99 lt 2 lw 2 lc rgb "black" front
	#set label '1.7x' at 310,98 font "Times-new-roman,20" front
	#set label '5%' at 225, 97 font "Times-new-roman,20" front 

	unset key

	#set arrow heads filled from 450,93.9 to 450,99.9 lt 0 lw 5 front


	plot "a_orig" using 2:1 title "Paxos" with lines dashtype 3 lw 4 lc rgb "red",\
		"a_rtop" using 2:1 title "Skyros" with lines dashtype 4 lw 4 lc rgb "blue" 


	unset multiplot

EOF
) | gnuplot -persist
