using System;
using System.Runtime.InteropServices;
using System.Threading;

class BytebeatPlayer
{
    // --- Configurações de Áudio ---
    const int SampleRate = 8000;
    const int BufferSize = 8000; // 1 segundo de áudio por buffer

    // --- Importação de DLLs do Windows (winmm.dll) ---
    [DllImport("winmm.dll")]
    public static extern int waveOutOpen(out IntPtr hWaveOut, int uDeviceID, ref WaveFormat lpFormat, IntPtr dwCallback, IntPtr dwInstance, int dwFlags);

    [DllImport("winmm.dll")]
    public static extern int waveOutPrepareHeader(IntPtr hWaveOut, ref WaveHeader lpWaveOutHdr, int uSize);

    [DllImport("winmm.dll")]
    public static extern int waveOutWrite(IntPtr hWaveOut, ref WaveHeader lpWaveOutHdr, int uSize);

    [DllImport("winmm.dll")]
    public static extern int waveOutUnprepareHeader(IntPtr hWaveOut, ref WaveHeader lpWaveOutHdr, int uSize);

    [StructLayout(LayoutKind.Sequential)]
    public struct WaveFormat
    {
        public short wFormatTag;
        public short nChannels;
        public int nSamplesPerSec;
        public int nAvgBytesPerSec;
        public short nBlockAlign;
        public short wBitsPerSample;
        public short cbSize;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct WaveHeader
    {
        public IntPtr lpData;
        public int dwBufferLength;
        public int dwBytesRecorded;
        public IntPtr dwUser;
        public int dwFlags;
        public int dwLoops;
        public IntPtr lpNext;
        public IntPtr reserved;
    }

    const int WHDR_DONE = 0x01;

    static void Main()
    {
        Console.WriteLine("Tocando Bytebeats... Pressione Ctrl+C para parar.");

        // Configura o formato: 8000Hz, 8-bit, Mono
        WaveFormat fmt = new WaveFormat();
        fmt.wFormatTag = 1; // PCM
        fmt.nChannels = 1;
        fmt.nSamplesPerSec = SampleRate;
        fmt.nAvgBytesPerSec = SampleRate;
        fmt.nBlockAlign = 1;
        fmt.wBitsPerSample = 8;
        fmt.cbSize = 0;

        if (waveOutOpen(out IntPtr hWaveOut, -1, ref fmt, IntPtr.Zero, IntPtr.Zero, 0) != 0)
        {
            Console.WriteLine("Erro ao abrir dispositivo de áudio.");
            return;
        }

        int t = 0;
        int mode = 0;
        DateTime startTime = DateTime.Now;

        // Loop principal
        while (true)
        {
            // Alterna o modo a cada 10 segundos
            if ((DateTime.Now - startTime).TotalSeconds > 10)
            {
                mode = (mode + 1) % 3;
                startTime = DateTime.Now;
            }

            // Gera os dados do Bytebeat
            byte[] buffer = new byte[BufferSize];
            for (int i = 0; i < BufferSize; i++)
            {
                buffer[i] = GenerateBytebeat(t, mode);
                t++;
            }

            // Aloca memória não gerenciada para o buffer (necessário para a API winmm)
            IntPtr pBuffer = Marshal.AllocHGlobal(BufferSize);
            Marshal.Copy(buffer, 0, pBuffer, BufferSize);

            WaveHeader header = new WaveHeader();
            header.lpData = pBuffer;
            header.dwBufferLength = BufferSize;

            waveOutPrepareHeader(hWaveOut, ref header, Marshal.SizeOf(header));
            waveOutWrite(hWaveOut, ref header, Marshal.SizeOf(header));

            // Espera o buffer terminar de tocar
            while ((header.dwFlags & WHDR_DONE) == 0)
            {
                Thread.Sleep(10);
            }

            waveOutUnprepareHeader(hWaveOut, ref header, Marshal.SizeOf(header));
            Marshal.FreeHGlobal(pBuffer);
        }
    }

    static byte GenerateBytebeat(int t, int mode)
    {
        switch (mode)
        {
            case 0:
                // Clássico "Lost"
                return (byte)(t * ((t >> 12 | t >> 8) & 63 & t >> 4));
            case 1:
                // Ritmo Industrial
                return (byte)(t * (t >> 8 | t >> 9) & 46 & t >> 8);
            case 2:
                // Glitch Espacial
                return (byte)((t * 5 & t >> 7) | (t * 3 & t >> 10));
            default:
                return (byte)(t * (t >> 5 | t >> 8));
        }
    }
}