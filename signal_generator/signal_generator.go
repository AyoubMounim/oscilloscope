package main

import "fmt"
import "flag"
import "math/rand"
import "time"
import "net"
import "encoding/binary"
import "math"

func normalizeFreq(freq_Hz float64, sampleFreq_Hz float64) float64 {
	_, normFreq := math.Modf(freq_Hz / sampleFreq_Hz)
	return normFreq
}

func sineWave(normalizedFreq float64, n int64) []float32 {
	res := make([]float32, 1)
	alpha := 2 * math.Pi * normalizedFreq * float64(n)
	normalized_alpha := alpha - 2*math.Pi*(math.Floor(alpha/2*math.Pi))
	res[0] = (float32(math.Sin(normalized_alpha)))
	return res
}

func expWave(normalizedFreq float64, n int64) []float32 {
	res := make([]float32, 2)
	alpha := 2 * math.Pi * normalizedFreq * float64(n)
	normalized_alpha := alpha - 2*math.Pi*(math.Floor(alpha/2*math.Pi))
	res[0] = (float32(math.Cos(normalized_alpha)))
	res[1] = (float32(math.Sin(normalized_alpha)))
	return res
}

func sineWaveNoisy(normalizedFreq float64, n int64, noiseLevel float64) float32 {
	alpha := 2 * math.Pi * normalizedFreq * float64(n)
	normalized_alpha := alpha - 2*math.Pi*(math.Floor(alpha/2*math.Pi))
	res := float32(math.Sin(normalized_alpha)) + float32(noiseLevel)*(2*rand.Float32()-1)
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

type numberGenFunc func(int64) []float32
type noiseFunc func() float32

func main() {
	ipFlag := flag.String("ip", "127.0.0.1:6969", "target ip address in the format: x.x.x.x:port")
	protoFlag := flag.String("proto", "tcp", "ip transport protocol to use: \"tcp\" or \"udp\"")
	waveFlag := flag.String("wave", "sine", "Wave form: \"sine\" \"exp\"")
	freqFlag := flag.Float64("freq", 1, "Wave frequency in Hz")
	noiseFlag := flag.Float64("noise", 0, "Noise level")
	sampleFreqFlag := flag.Float64("sampleFreq", 100000, "Sample frequency in Hz")
	flag.Parse()

	conn, err := net.Dial(*protoFlag, *ipFlag)
	if err != nil {
		fmt.Println(err)
		return
	}

	var n int64 = 0
	deltaT := time.Duration(float64(1000000000) / (*sampleFreqFlag))
	normFreq := normalizeFreq(*freqFlag, *sampleFreqFlag)
	var numberGen numberGenFunc
	var noise noiseFunc
	switch *waveFlag {
	case "sine":
		numberGen = func(n int64) []float32 { return sineWave(normFreq, n) }
	case "exp":
		numberGen = func(n int64) []float32 { return expWave(normFreq, n) }
	default:
		numberGen = func(n int64) []float32 { return sineWave(normFreq, n) }
	}
	if *noiseFlag == 0 {
		noise = func() float32 { return 0 }
	} else {
		noise = func() float32 { return float32(*noiseFlag) * (2*rand.Float32() - 1) }
	}
	fmt.Printf("Sending data...")
	for {
		numbers := numberGen(n)
		for _, n := range numbers {
			n += noise()
			err := binary.Write(conn, binary.LittleEndian, n)
			if err != nil {
				panic(err)
			}
		}
		time.Sleep(deltaT)
		n += 1
	}
}
