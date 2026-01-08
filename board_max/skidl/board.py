from skidl import *

# ==========================================
# SEARCH PATH CONFIGURATION
# ==========================================
# Replace these with the actual folder paths containing your .kicad_sym files
lib_search_paths[KICAD].append('/home/bill/src/throw_away/libraries')
lib_search_paths[KICAD].append('/home/bill/src/throw_away/libraries/espressif_library/symbols')

# ==========================================
# 1. ESP32-S3-DevKitC-1 FULL DEFINITION
# ==========================================
# Using 'Espressif' library nickname
esp32 = Part('Espressif', 'ESP32-S3-DevKitC', footprint='Custom_Footprints:ESP32-S3-DevKitC')

# Header J1 (Pins 1-22)
esp32[1].aliases  += '3V3_1'
esp32[2].aliases  += '3V3_2'
esp32[3].aliases  += 'EN', 'RST'
esp32[4].aliases  += 'GPIO4'
esp32[5].aliases  += 'GPIO5'
esp32[6].aliases  += 'GPIO6'
esp32[7].aliases  += 'GPIO7'
esp32[8].aliases  += 'GPIO15'
esp32[9].aliases  += 'GPIO16'
esp32[10].aliases += 'GPIO17'
esp32[11].aliases += 'GPIO18'
esp32[12].aliases += 'GPIO8'
esp32[13].aliases += 'GPIO3'
esp32[14].aliases += 'GPIO46'
esp32[15].aliases += 'GPIO9'
esp32[16].aliases += 'GPIO10'
esp32[17].aliases += 'GPIO11', 'FSPID'   # SPI MOSI
esp32[18].aliases += 'GPIO12', 'FSPICLK' # SPI SCLK
esp32[19].aliases += 'GPIO13'
esp32[20].aliases += 'GPIO14'
esp32[21].aliases += '5V'
esp32[22].aliases += 'GND_1'

# Header J3 (Pins 23-44)
esp32[23].aliases += 'GND_2'
esp32[24].aliases += 'TX', 'GPIO43'
esp32[25].aliases += 'RX', 'GPIO44'
esp32[26].aliases += 'GPIO1'
esp32[27].aliases += 'GPIO2'
esp32[28].aliases += 'GPIO42'
esp32[29].aliases += 'GPIO41'
esp32[30].aliases += 'GPIO40'
esp32[31].aliases += 'GPIO39'
esp32[32].aliases += 'GPIO38'
esp32[33].aliases += 'GPIO37'
esp32[34].aliases += 'GPIO36'
esp32[35].aliases += 'GPIO35'
esp32[36].aliases += 'GPIO0'
esp32[37].aliases += 'GPIO45'
esp32[38].aliases += 'GPIO48'
esp32[39].aliases += 'GPIO47'
esp32[40].aliases += 'GPIO21'
esp32[41].aliases += 'GPIO20', 'USB_D+'
esp32[42].aliases += 'GPIO19', 'USB_D-'
esp32[43].aliases += 'GND_3'
esp32[44].aliases += 'GND_4'

# ==========================================
# 2. MAX4820 TSSOP FULL DEFINITION
# ==========================================
# Using 'MAX4820EUP_T' library nickname
drivers = [Part('MAX4820EUP_T', 'MAX4820', footprint='Package_SO:TSSOP-20_4.4x6.5mm_P0.65mm') for _ in range(5)]

for drv in drivers:
    drv[1].aliases  += 'VCC'
    drv[2].aliases  += 'SET'
    drv[3].aliases  += 'RESET'
    drv[4].aliases  += 'CS'
    drv[5].aliases  += 'DIN'
    drv[6].aliases  += 'SCLK'
    drv[7].aliases  += 'DOUT'
    drv[8].aliases  += 'NC'
    drv[9].aliases  += 'GND'
    drv[10].aliases += 'OUT8'
    drv[11].aliases += 'OUT7'
    drv[12].aliases += 'PGND1'
    drv[13].aliases += 'OUT6'
    drv[14].aliases += 'OUT5'
    drv[15].aliases += 'COM'
    drv[16].aliases += 'OUT4'
    drv[17].aliases += 'OUT3'
    drv[18].aliases += 'PGND2'
    drv[19].aliases += 'OUT2'
    drv[20].aliases += 'OUT1'

# ==========================================
# 3. CONNECTIONS
# ==========================================

vcc_3v3 = Net('3V3')
gnd = Net('GND')

# Connect MCU Power and Ground
esp32['3V3_1', '3V3_2'] += vcc_3v3
gnd += esp32.filter_pins(name='GND_.*')

# Control Nets
spi_clk = Net('SPI_SCLK')
spi_mosi = Net('SPI_DIN')
sys_set = Net('RELAY_SET')
sys_reset = Net('RELAY_RESET')

spi_clk += esp32['FSPICLK']
spi_mosi += esp32['FSPID']
sys_set += esp32['GPIO2']
sys_reset += esp32['GPIO42']

# Individual Chip Selects
drivers[0]['CS'] += esp32['GPIO10']
drivers[1]['CS'] += esp32['GPIO13']
drivers[2]['CS'] += esp32['GPIO14']
drivers[3]['CS'] += esp32['GPIO21']
drivers[4]['CS'] += esp32['GPIO1']

# Connect common pins for each driver
for drv in drivers:
    drv['VCC'] += vcc_3v3
    drv['GND', 'PGND1', 'PGND2'] += gnd
    drv['COM'] += vcc_3v3
    drv['SCLK'] += spi_clk
    drv['DIN'] += spi_mosi
    drv['SET'] += sys_set
    drv['RESET'] += sys_reset
    
    # 10k Pull-ups for SET and RESET
    r_set = Part('Device', 'R', value='10k', footprint='Resistor_SMD:R_0603_1608Metric')
    r_reset = Part('Device', 'R', value='10k', footprint='Resistor_SMD:R_0603_1608Metric')
    r_set[1, 2] += vcc_3v3, drv['SET']
    r_reset[1, 2] += vcc_3v3, drv['RESET']

    # 0.1uF Local Bypass Capacitor
    cap = Part('Device', 'C', value='0.1uF', footprint='Capacitor_SMD:C_0603_1608Metric')
    cap[1, 2] += vcc_3v3, gnd

generate_netlist()
