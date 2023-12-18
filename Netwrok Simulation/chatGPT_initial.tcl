# Create a new simulation instance
set ns [new Simulator]

# Create two routers
set R1 [$ns node]
set R2 [$ns node]

# Create two senders connected to R1
set src1 [$ns node]
set src2 [$ns node]

# Create two receivers connected to R2
set rcv1 [$ns node]
set rcv2 [$ns node]

# Set link properties
$ns duplex-link $R1 $R2 1Mbps 5ms DropTail
$ns duplex-link $src1 $R1 10Mbps 0ms DropTail
$ns duplex-link $src2 $R1 10Mbps 0ms DropTail
$ns duplex-link $rcv1 $R2 10Mbps 0ms DropTail
$ns duplex-link $rcv2 $R2 10Mbps 0ms DropTail

# Set up FTP applications on senders
set ftp1 [new Application/FTP]
set ftp2 [new Application/FTP]

# Attach FTP applications to agents on src1 and src2
$ftp1 attach-agent [new Agent/TCP]
$ftp2 attach-agent [new Agent/TCP]

$ns attach-agent $src1 $ftp1
$ns attach-agent $src2 $ftp2

# Connect FTP agents to receivers
$ns connect [new Agent/TCP] $rcv1
$ns connect [new Agent/TCP] $rcv2

# Set simulation end time
$ns at 10.0 "$ns halt"

# Run the simulation
$ns run
