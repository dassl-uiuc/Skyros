#! /usr/bin/env python

from zplot import *

if len(sys.argv) != 2:
	print 'Usage: program indir'
	sys.exit(1)

indir = sys.argv[1]
codes = ['orig', 'rtop']

outfile = indir + '/ycsb-thrpt.eps'
c = postscript(title=outfile,  dimensions=[320, 170])
d = drawable(canvas=c, xrange=[0,165], yrange=[0,140], coord=[56,30], dimensions=[270,112])
tables = []
for code in codes:
	infile = indir + '/' + code
	tables.append(table(file=infile))
	tables[-1].addcolumn(column='zero', value=0.1)
	tables[-1].addcolumn(column='one',  value=425)

# because tics and axes are different, call AxesTicsLabels twice, once to specify x-axis, other to specify y-axis

axis(drawable=d, style='y', doyminortics=False, yauto=[0,130,30], ytitle='Throughput (Kops/s)', ytitleshift = [-1.8,-2], ytitlesize=16, ylabelfontsize=16.0)
axis(drawable=d, style='x', xmanual = [["",0]])

p = plotter()
L = legend()
lw = 0.1
bw = 8
p.points(drawable=d, table=tables[0], xfield='c0', yfield='zero', style='label', labelfield='c1', labelanchor='c,c',labelsize=16, labelshift=[7,-13])

p.verticalbars(drawable=d, table=tables[0], xfield='c0', fill=True, barwidth=bw, yfield='c3', fillcolor='gray', linewidth=lw, legend=L, legendtext='Paxos')
p.verticalbars(drawable=d, table=tables[1], xfield='c0', fill=True, barwidth=bw, yfield='c3', fillcolor='black', fillstyle='dline1', fillsize=1.25, fillskip=5, linewidth=lw, legend=L, legendtext='Skyros')


for i in [1]: 
	p.points(drawable=d, table=tables[i], xfield='c0', yfield='c3',  style='label', labelfield='c2', labelanchor='l,c', labelsize=16.0, labelshift=[-17, 16])
	pass
L.draw(drawable=d, coord=[0,165],down=False, skipnext=1, skipspace=70, fontsize=16, width=12, height=12)

c.render()
