# 带局部性的地址序列: 50 个阶段, 每阶段 20 次访问, 每阶段从 6 个热点页中取页
x = 2024


def rnd(n):
    global x
    x = (1103515245 * x + 12345) % 2**31
    return (x >> 16) % n


addrs = []
for _ in range(50):
    hot = [rnd(256) for _ in range(6)]
    for _ in range(20):
        addrs.append(hot[rnd(6)] * 256 + rnd(256))
print("\n".join(map(str, addrs)))
