#file from mniip, https://github.com/mniip/The-Powder-Toy/commit/d46d9f3f815d
import sys
def encode(value):
	if value == ord('\n'):
		return '\\n'
	if value == ord('"'):
		return '\\"'
	if value == ord('\\'):
		return '\\\\'
	if 32 <= value <= 126:
		return chr(value)
	return '\\{0:03o}'.format(value)

f = open(sys.argv[2], 'rb')

data = f.read()
f.close()
size = len(data)
data = '"' + ''.join(encode(value) for value in data) + '"'

i = open(sys.argv[3], 'r')
o = open(sys.argv[1], 'w')
o.write(i.read().replace('/*#SIZE*/', str(size)).replace('/*#DATA*/', data))
i.close()
o.close()
