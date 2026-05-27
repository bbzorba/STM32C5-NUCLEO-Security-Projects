param([string]$Port = "COM8", [int]$Baud = 115200, [int]$TimeoutMs = 8000)

$port = New-Object System.IO.Ports.SerialPort $Port,$Baud,'None',8,'One'
$port.Handshake = 'None'
$port.DtrEnable = $true
$port.RtsEnable = $true
$port.NewLine = "`r`n"
$port.ReadTimeout = 100
$port.Open()
Write-Host "Port opened. Resetting device..."

& "C:/Program Files/STMicroelectronics/STM32Cube/STM32CubeProgrammer/bin/STM32_Programmer_CLI.exe" -c port=SWD -rst | Out-Null

$output = ""
$sw = [System.Diagnostics.Stopwatch]::StartNew()
while ($sw.ElapsedMilliseconds -lt $TimeoutMs) {
    try {
        $line = $port.ReadLine()
        Write-Host $line
        $output += $line + "`n"
    } catch { }
}
$port.Close()
Write-Host "--- Done ---"
