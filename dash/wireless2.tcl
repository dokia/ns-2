# Copyright (c) 1997 Regents of the University of California.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. All advertising materials mentioning features or use of this software
#    must display the following acknowledgement:
#      This product includes software developed by the Computer Systems
#      Engineering Group at Lawrence Berkeley Laboratory.
# 4. Neither the name of the University nor of the Laboratory may be used
#    to endorse or promote products derived from this software without
#    specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.
#
# wireless2.tcl
# simulation of a wired-cum-wireless scenario consisting of 2 wired nodes
# connected to a wireless domain through a base-station node.
# ======================================================================
# Define options
# ======================================================================
set opt(chan)           Channel/WirelessChannel    ;# channel type
#set opt(prop)           Propagation/TwoRayGround   ;# radio-propagation model
set opt(prop)           Propagation/Shadowing   ;# radio-propagation model
set opt(netif)          Phy/WirelessPhy            ;# network interface type
set opt(mac)            Mac/802_11                 ;# MAC type
set opt(ifq)            Queue/DropTail/PriQueue    ;# interface queue type
set opt(ll)             LL                         ;# link layer type
set opt(ant)            Antenna/OmniAntenna        ;# antenna model
set opt(ifqlen)         50                         ;# max packet in ifq
set opt(nn)             2                          ;# number of mobilenodes
set opt(adhocRouting)   DSDV                       ;# routing protocol

set opt(cp)             ""                         ;# connection pattern file
set opt(sc)     	""    			   ;# node movement file. 

set opt(x)      670                            ;# x coordinate of topology
set opt(y)      670                            ;# y coordinate of topology
set opt(seed)   0.0                            ;# seed for random number gen.
set opt(stop)   250                            ;# time to stop simulation

set opt(ftp1-start)      2.0
set opt(ftp2-start)      1.0

set num_wired_nodes      5
set num_bs_nodes         1

# first set values of shadowing model
Propagation/Shadowing set pathlossExp_ 1.0  ;# path loss exponent
Propagation/Shadowing set std_db_ 4.0       ;# shadowing deviation (dB)
Propagation/Shadowing set dist0_ 1.0        ;# reference distance (m)
Propagation/Shadowing set seed_ 0           ;# seed for RNG

Mac/802_11 set dataRate_ 11Mb
Mac/802_11 set RTSThreshold_ 3000
Phy/WirelessPhy set noise_power_ 1.7e-14
#set macobj Mac/802_11
#Mac/802_11 set frame_loss_ratio_ 0;
# ============================================================================
# check for boundary parameters and random seed
if { $opt(x) == 0 || $opt(y) == 0 } {
	puts "No X-Y boundary values given for wireless topology\n"
}
if {$opt(seed) > 0} {
	puts "Seeding Random number generator with $opt(seed)\n"
	ns-random $opt(seed)
}

# create simulator instance
set ns_   [new Simulator]

# set up for hierarchical routing
$ns_ node-config -addressType hierarchical
AddrParams set domain_num_ 2           ;# number of domains
lappend cluster_num 5 1                ;# number of clusters in each domain
AddrParams set cluster_num_ $cluster_num
lappend eilastlevel 1 1 1 1 1 4              ;# number of nodes in each cluster 
AddrParams set nodes_num_ $eilastlevel ;# of each domain

set fth_ [open throughput.tr w]	;# trace file for throughput
set fbu_ [open buffer.tr w]	;# trace file for buffer size
set fbr_ [open bitrate.tr w]	;# trace file for bitrate
set flr_ [open lossrate.tr w]	;# trace file for wired loss rate
set flrw_ [open lossratewireless.tr w]	;# trace file for wireless loss rate
set fpt_ [open playtime.tr w]    ;# trace file for play time
set fmd_ [open macdelay.tr w]	;# trace for wireless mac delay

set tracefd  [open wireless2-out.tr w]
set namtrace [open wireless2-out.nam w]
$ns_ trace-all $tracefd
$ns_ namtrace-all-wireless $namtrace $opt(x) $opt(y)

# Create topography object
set topo   [new Topography]

# define topology
$topo load_flatgrid $opt(x) $opt(y)

# create God
create-god [expr $opt(nn) + $num_bs_nodes]

#create wired nodes
set temp {0.0.0 0.1.0 0.2.0 0.3.0 0.4.0}        ;# hierarchical addresses for wired domain
for {set i 0} {$i < $num_wired_nodes} {incr i} {
    set W($i) [$ns_ node [lindex $temp $i]]
}

# use full tcp
Agent/DSDV set fulltcp_ 1
# configure for base-station node
#-propType Propagation/Shadowing
#-channelType $opt(chan) \

set chan_1_ [new $opt(chan)]
 
$ns_ node-config -adhocRouting $opt(adhocRouting) \
                 -llType $opt(ll) \
                 -macType $opt(mac) \
                 -ifqType $opt(ifq) \
                 -ifqLen $opt(ifqlen) \
                 -antType $opt(ant) \
		 -propType $opt(prop) \
                 -phyType $opt(netif) \
		 -channel $chan_1_ \
		 -topoInstance $topo \
                 -wiredRouting ON \
		 -agentTrace ON \
                 -routerTrace OFF \
                 -macTrace OFF 

#create base-station node
set temp {1.0.0 1.0.1 1.0.2 1.0.3 1.0.4 1.0.5 1.0.6 1.0.7}   ;# hier address to be used for wireless
                                     ;# domain
set BS(0) [$ns_ node [lindex $temp 0]]
$BS(0) random-motion 0               ;# disable random motion

#provide some co-ord (fixed) to base station node
$BS(0) set X_ 1.0
$BS(0) set Y_ 2.0
$BS(0) set Z_ 0.0

# create mobilenodes in the same domain as BS(0)  
# note the position and movement of mobilenodes is as defined
# in $opt(sc)

#configure for mobilenodes
$ns_ node-config -wiredRouting OFF

for {set j 0} {$j < $opt(nn)} {incr j} {
    set node_($j) [ $ns_ node [lindex $temp \
	    [expr $j+1]] ]
    $node_($j) base-station [AddrParams addr2id \
	    [$BS(0) node-addr]]
}

# set the position of node_(0)
$node_(0) set X_ 1.0
$node_(0) set Y_ 102.0
$node_(0) set Z_ 0.0

#create links between wired and BS nodes

#$ns_ duplex-link $W(0) $W(1) 5Mb 2ms DropTail
#$ns_ duplex-link $W(1) $BS(0) 5Mb 2ms DropTail
$ns_ duplex-link $W(0) $W(1) 5Mb 2ms RED
$ns_ duplex-link $W(1) $W(2) 1Mb 2ms RED
$ns_ duplex-link $W(1) $W(3) 5Mb 2ms RED
$ns_ duplex-link $W(2) $W(4) 5Mb 2ms RED
$ns_ duplex-link $W(2) $BS(0) 5Mb 2ms RED

# queue monitoring
$ns_ duplex-link-op $W(1) $W(2) queuePos 0.5

# Tracing a queue
set redq [[$ns_ link $W(1) $W(2)] queue]
set ll [$ns_ link $BS(0) $node_(0)]

#$ns_ duplex-link-op $W(0) $W(1) orient down
#$ns_ duplex-link-op $W(1) $BS(0) orient left-down

# setup TCP connections
set tcp1 [new Agent/TCP/FullTcp]
$tcp1 set class_ 2
set sink1 [new Agent/TCP/FullTcp]
$ns_ attach-agent $node_(0) $tcp1
$ns_ attach-agent $W(0) $sink1
#$ns_ attach-agent $BS(0) $sink1
$tcp1 set fid_ 0;
$sink1 set fid_ 0;
$ns_ connect $tcp1 $sink1

$sink1 listen;
$tcp1 set window_ 100;

set client [new Application/DashAppClient]
$client bitratelist 50 100 200 500 1000 2000 5000 10000 20000 50000 100000 200000 500000
#$client bitratelist 100000
$client set playrate 0
$client attach-agent $tcp1

set server [new Application/DashAppServer]
$server bitratelist 50 100 200 500 1000 2000 5000 10000 20000 50000 100000 200000 500000
#$server bitratelist 100000
$server attach-agent $sink1
$server set segnumber 100
$server set seglength 1
$ns_ at $opt(ftp1-start) "$client start"
#$ns_ at $opt(ftp1-start) "$tcp1 sendmsg 1000 0"
############ play control #########################
#$ns_ at 60.0 "$client pauseplay"
#$ns_ at 70.0 "$client backtoplay"
#$ns_ at 10.0 "$client jumpto 30"
#$ns_ at 80.0 "$client jumpto 30"

#set ftp1 [new Application/FTP]
#$ftp1 attach-agent $tcp1
#$ns_ at $opt(ftp1-start) "$ftp1 send 2000"


#set tcp2 [new Agent/TCP]
#$tcp2 set class_ 2
#set sink2 [new Agent/TCPSink]
#$ns_ attach-agent $W(1) $tcp2
#$ns_ attach-agent $node_(2) $sink2
#$ns_ connect $tcp2 $sink2
#set ftp2 [new Application/FTP]
#$ftp2 attach-agent $tcp2
#$ns_ at $opt(ftp2-start) "$ftp2 start"

# set traffic background (backbone)

set udp0 [new Agent/UDP]
set lossmonitor0 [new Agent/LossMonitor]
$ns_ attach-agent $W(3) $udp0
$ns_ attach-agent $W(4) $lossmonitor0

$udp0 set class_ 1

$ns_ color 1 Blue 	;#udp
#$ns_ color 2 Red	;#tcp

set inter_ 0.08  ;# no smaller than 0.005

set cbr0 [new Application/Traffic/CBR]
$cbr0 set packetSize_ 500
$cbr0 set interval_ $inter_
$cbr0 attach-agent $udp0
$ns_ connect $udp0 $lossmonitor0
$ns_ at $opt(ftp2-start) "$cbr0 start"

# opposite direction traffic
set udp1 [new Agent/UDP]
set lossmonitor1 [new Agent/LossMonitor]
$ns_ attach-agent $W(4) $udp1
$ns_ attach-agent $W(3) $lossmonitor1

$udp1 set class_ 3

$ns_ color 3 Red 	;#udp

set cbr1 [new Application/Traffic/CBR]
$cbr1 set packetSize_ 500
$cbr1 set interval_ $inter_
$cbr1 attach-agent $udp0
$ns_ connect $udp1 $lossmonitor1
$ns_ at $opt(ftp2-start) "$cbr1 start"


# set traffic background (mobile nodes)
for {set j 1} {$j < $opt(nn)} {incr j} {
	set udp($j) [new Agent/UDP]
	set null($j) [new Agent/Null]
	$ns_ attach-agent $node_($j) $udp($j)
	$ns_ attach-agent $W(1) $null($j)
	$ns_ connect $udp($j) $null($j)
	set cbr($j) [new Application/Traffic/CBR]
	$cbr($j) set packetSize_ 1000
	$cbr($j) set interval_ 0.08;
	$cbr($j) attach-agent $udp($j)
	$ns_ at $opt(ftp2-start) "$cbr($j) start"
	#set tcp($j) [new Agent/TCP]
	#set sink($j) [new Agent/TCPSink]
	#$ns_ attach-agent $W(1) $tcp($j)
	#$ns_ attach-agent $node_($j) $sink($j)
	#$ns_ connect $tcp($j) $sink($j)
	#set ftp($j) [new Application/FTP]
	#$ftp($j) attach-agent $tcp($j)
	#$ns_ at $opt(ftp2-start) "$ftp($j) start"
}

# source connection-pattern and node-movement scripts
if { $opt(cp) == "" } {
	puts "*** NOTE: no connection pattern specified."
        set opt(cp) "none"
} else {
	puts "Loading connection pattern..."
	source $opt(cp)
}
if { $opt(sc) == "" } {
	puts "*** NOTE: no scenario file specified."
        set opt(sc) "none"
} else {
	puts "Loading scenario file..."
	source $opt(sc)
	puts "Load complete..."
}

# Define initial node position in nam

for {set i 0} {$i < $opt(nn)} {incr i} {

    # 20 defines the node size in nam, must adjust it according to your
    # scenario
    # The function must be called after mobility model is defined

    $ns_ initial_node_pos $node_($i) 20
}     

# Tell all nodes when the simulation ends
for {set i } {$i < $opt(nn) } {incr i} {
    $ns_ at $opt(stop).0 "$node_($i) reset";
}
$ns_ at $opt(stop).0 "$BS(0) reset";

$ns_ at $opt(stop).0002 "puts \"NS EXITING...\" ; $ns_ halt"
$ns_ at $opt(stop).0001 "stop"
proc stop {} {
    global ns_ tracefd namtrace fth_ fbu_ fbr_ flr_ fpt_ flrw_ fmd_
#    $ns_ flush-trace
    close $tracefd
    close $namtrace
    close $fth_
    close $fbu_
    close $fbr_
    close $flr_
    close $fpt_
    close $flrw_
    close $fmd_
    exec xgraph throughput.tr -geometry 800x400 &
    exec xgraph buffer.tr -geometry 800x400 &
    exec xgraph bitrate.tr -geometry 800x400 &
    exec xgraph lossrate.tr -geometry 800x400 &
    exec xgraph playtime.tr -geometry 800x400 &
    exec xgraph lossratewireless.tr -geometry 800x400 &
    exec xgraph macdelay.tr -geometry 800x400 &
    exit 0
}

proc play {} {
    global client
    set ns [Simulator instance]
    set readsize 0.06
    set now [$ns now]
    $client readbuffer $readsize
    
    $ns at [expr $now+$readsize] "play"
}

proc record {} {
     global sink1 client server redq fth_ fbu_ fbr_ flr_ fpt_ flrw_ fmd_
     set ns [Simulator instance]
     #set interval 2.0
     set interval 0.6
     #$client readbuffer 0.06

     set bw0 [$client set bytes]
     set buffer [$client set buffer]
     set bitrate [$server set bitrate]
     #set loss [$lossmonitor0 set nlost_]
     #set loss [$redq set pkt_drop_ptg_]
     set loss [$sink1 set packet_loss_ratio_]
     #set lossw [Mac/802_11 set dataRate_] 
     set lossw [Mac/802_11 set frame_loss_ratio_] 
     set pt [$client set playtime]
     set macdelay [Mac/802_11 set delay_]
     set now [$ns now]
     puts $fth_ "$now [expr $bw0/$interval*8]"
     puts $fbu_ "$now $buffer"
     puts $fbr_ "$now $bitrate"
     puts $flr_ "$now [expr $loss]"
     puts $fpt_ "$now $pt"
     puts $flrw_ "$now $lossw"
     puts $fmd_ "$now $macdelay"
     #$client set bytes 0 
     #$lossmonitor0 set nlost_ 0
     $ns at [expr $now+$interval] "record"
}
$ns_ at 0.0 "record"
$ns_ at 0.0 "play"

# informative headers for CMUTracefile
puts $tracefd "M 0.0 nn $opt(nn) x $opt(x) y $opt(y) rp \
	$opt(adhocRouting)"
puts $tracefd "M 0.0 sc $opt(sc) cp $opt(cp) seed $opt(seed)"
puts $tracefd "M 0.0 prop $opt(prop) ant $opt(ant)"

puts "Starting Simulation..."
$ns_ run

