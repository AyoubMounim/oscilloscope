package main

import "fmt"
import "flag"
import "math/rand"
import "time"
import "net"
import "encoding/binary"
import "bytes"
import "math"

var deltaT time.Duration = 10 * time.Microsecond

func sinus(time_seconds float64) float32 {
	freq_Hz := 1000.
	alpha := 2 * math.Pi * freq_Hz * time_seconds
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
	flag.Parse()
	conn, err := net.Dial("udp", *ipFlag)
	if err != nil {
		fmt.Println(err)
		return
	}
	var buf bytes.Buffer
	fmt.Printf("Sending data...")
	step := 0
	for {
		//number := float32(0.5)
		//number := random()
		//number := sinus(float64(step) * deltaT.Seconds())
		//number := noise(float64(step) * deltaT.Seconds())
		number := armonic(float64(step)*deltaT.Seconds(), 3)
		err := binary.Write(&buf, binary.BigEndian, number)
		if err != nil {
			fmt.Println(err)
			continue
		}
		var data [4]byte
		_, err = buf.Read(data[:])
		if err != nil {
			fmt.Println(err)
			continue
		}
		conn.Write(data[:])
		time.Sleep(deltaT)
		step += 1
	}
}
