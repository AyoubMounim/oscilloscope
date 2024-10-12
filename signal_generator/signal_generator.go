package main

import "fmt"
import "flag"
import "math/rand"
import "time"
import "net"
import "encoding/binary"
import "math"

func normalizeFreq(freq_Hz float64, deltaT_seconds float64) float64 {
	_, normFreq := math.Modf(freq_Hz * deltaT_seconds)
	return normFreq
}

func sineWave(normalizedFreq float64, n int64) float32 {
	alpha := 2 * math.Pi * normalizedFreq * float64(n)
	normalized_alpha := alpha - 2*math.Pi*(math.Floor(alpha/2*math.Pi))
	res := float32(math.Sin(normalized_alpha))
	return res
}

func armonic(time_seconds float64, n int) float32 {
	freq_Hz := 1000.
	res := 0.0
	for i := 1; i <= n; i++ {
		alpha := 2 * math.Pi * freq_Hz * float64(i) * time_seconds
		normalized_alpha := alpha - 2*math.Pi*(math.Floor(alpha/2*math.Pi))
		res += math.Sin(normalized_alpha)
	}
	return float32(res)
}

func noise(time_seconds float64) float32 {
	freq_Hz := 1000.
	alpha := 2 * math.Pi * freq_Hz * time_seconds
	normalized_alpha := alpha - 2*math.Pi*(math.Floor(alpha/2*math.Pi))
	res := float32(math.Sin(normalized_alpha)) + 2*rand.Float32() - 1
	return res
}

func random() float32 {
	return rand.Float32()
}

func main() {
	ipFlag := flag.String("ip", "127.0.0.1:6969", "target ip address in the format: x.x.x.x:port")
	protoFlag := flag.String("proto", "tcp", "ip transport protocol to use: \"tcp\" or \"udp\"")
	sineFlag := flag.Float64("sine", 1, "Sine wave frequency in Hz")
	deltaTFlag := flag.Int64("deltaT", 10, "Quantized time step in microseconds")
	flag.Parse()

	conn, err := net.Dial(*protoFlag, *ipFlag)
	if err != nil {
		fmt.Println(err)
		return
	}

	var n int64 = 0
	deltaT := time.Duration(*deltaTFlag * 1000)
	normFreq := normalizeFreq(*sineFlag, float64(deltaT.Seconds()))
	fmt.Printf("Sending data...")
	for {
		//number := float32(0.5)
		//number := random()
		number := sineWave(normFreq, n)
		//number := noise(float64(step) * deltaT.Seconds())
		//number := armonic(float64(step)*deltaT.Seconds(), 3)
		err := binary.Write(conn, binary.BigEndian, number)
		if err != nil {
			fmt.Println(err)
			continue
		}
		time.Sleep(deltaT)
		n += 1
	}
}
