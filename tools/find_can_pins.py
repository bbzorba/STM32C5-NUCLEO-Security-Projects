import fitz, re, sys
path=r'd:/baris/personal/personal_projects/STM32C5-NUCLEO_Projects/docs/STM32C5 Nucleo-64 board user manual.pdf'
try:
    doc=fitz.open(path)
except Exception as e:
    print('OPEN_ERR', e); sys.exit(1)
pat=re.compile(r'FDCAN|CANH|CANL|PA11|PA12|CN\d+|Arduino|Morpho|connector|CAN', re.I)
for i in range(len(doc)):
    txt=doc[i].get_text()
    if pat.search(txt):
        print('---PAGE', i+1)
        s=txt[:2000]
        s=re.sub(r'\s+', ' ', s)
        print(s)
        print()
