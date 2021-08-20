#! /usr/bin/env python

from zplot import *

if len(sys.argv) != 2:
	print 'Usage: program indir'
	sys.exit(1)

indir = sys.argv[1]
codes = ['orig', 'curp', 'rtop']

outfile = indir + '/fscurp.eps'
c = postscript(title=outfile,  dimensions=[200, 170])
d = drawable(canvas=c, xrange=[0,54], yrange=[0,50], coord=[55,27], dimensions=[150,121])
tables = []
for code in codes:
	infile = indir + '/' + code
	tables.append(table(file=infile))
	tables[-1].addcolumn(column='zero', value=0.1)
	tables[-1].addcolumn(column='one',  value=425)

# because tics and axes are different, call AxesTicsLabels twice, once to specify x-axis, other to specify y-axis

axis(drawable=d, style='y', doyminortics=False, yauto=[0,50,10], ytitle='Throughput (Kops/sec)', ytitleshift = [-5,-2], ytitlesize=14, ylabelfontsize=14.0)
axis(drawable=d, style='x', xmanual = [["",0]])

p = plotter()
L = legend()
lw = 0.1
bw = 8
p.points(drawable=d, table=tables[0], xfield='c0', yfield='zero', style='label', labelfield='c1', labelanchor='c,c',labelsize=14, labelshift=[0,-13])
p.points(drawable=d, table=tables[1], xfield='c0', yfield='zero', style='label', labelfield='c1', labelanchor='c,c',labelsize=14, labelshift=[0,-13])
p.points(drawable=d, table=tables[2], xfield='c0', yfield='zero', style='label', labelfield='c1', labelanchor='c,c',labelsize=14, labelshift=[0,-13])

p.verticalbars(drawable=d, table=tables[0], xfield='c0', fill=True, barwidth=bw, yfield='c3', fillcolor='gray', linewidth=lw, legend=L, legendtext='Paxos')
p.verticalbars(drawable=d, table=tables[2], xfield='c0', fill=True, barwidth=bw, yfield='c3', fillcolor='black', fillstyle='dline1', fillsize=1.25, fillskip=5, linewidth=lw, legend=L, legendtext='Skyros')
p.verticalbars(drawable=d, table=tables[1], xfield='c0', fill=True, barwidth=bw, yfield='c3', fillcolor='black', linewidth=lw, legend=L, legendtext='Curp-c')

for i in [0, 1, 2]: 
	p.points(drawable=d, table=tables[i], xfield='c0', yfield='c3',  style='label', labelfield='c2', labelanchor='l,c', labelsize=14.0, labelshift=[-10, 12])
	pass

c.render()
