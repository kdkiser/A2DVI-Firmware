import sys

with open("font.BIN", "rb") as file:
    with open("font-inv.txt", "w") as output:
        data = file.read()
        fontHeight = 0x10

        for idx, d in enumerate(data):
            if idx % fontHeight == 0:
                if idx != 0:
                    output.writelines('\n')
                    print()
                print("// character " + hex(idx//fontHeight))
                output.writelines("\t// character " + hex(idx//fontHeight) + '\n')
            #out = bin(d)
            out = '{0:08b}'.format(d)
            out = out[::-1]
            
            out = out.replace('0', 'x')
            out = out.replace('1', '0')
            out = out.replace('x', '1')

            out = '0b' + out + ','
            #out.zfill(8)
            #out = "0b" + out
            print(out)
            output.writelines('\t' + out + '\n')

output.close()
file.close()
