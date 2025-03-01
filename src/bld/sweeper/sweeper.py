sweeper=[]
with open("./sweeper.txt", "r") as f:
    lines = f.readlines()
    for line in lines:
        if(line.strip()!=""):
            sweeper.append(list(line.strip()))

with open("./sweeper.bin", "wb") as f:
    for line in sweeper:
        for b in line:
            if(b=="0"):
                f.write(b'\x01')
            else:
                f.write(b'\x02')


def print_map():
    header="   "
    for x in range(0,30):
        header+=f"{str(x).rjust(2, ' ')} "

    print(header)
    print("")

    for i, line in enumerate(sweeper):
        print(f"{str(i).rjust(2, ' ')}  "+"  ".join(line))

hits=[[12, 14], [24, 14], [21, 9], [17, 3], [14, 14], [23, 5], [23, 2], [23, 10], [1, 11], [3, 5], [22, 6], [27, 13], [8, 13], [17, 8], [28, 12], [1, 0], [8, 15], [25, 15], [24, 3], [13, 4], [23, 7], [7, 2], [26, 14], [11, 5], [2, 7], [18, 7], [11, 4], [11, 11], [8, 8], [16, 15], [19, 15], [13, 9], [13, 14], [29, 9], [26, 12], [25, 6], [5, 12], [19, 11], [14, 11], [22, 12], [9, 0], [27, 11], [11, 9], [8, 12], [27, 12], [6, 3], [9, 8], [1, 5], [26, 3], [26, 11], [15, 12], [29, 1], [9, 13], [13, 10], [2, 9], [24, 6], [14, 7], [27, 4], [0, 6], [19, 10], [7, 9], [16, 2], [4, 9], [15, 13], [0, 9], [9, 10], [14, 0], [0, 3], [27, 3], [2, 14], [28, 13], [25, 11], [10, 4], [25, 1], [19, 7], [14, 6], [29, 14], [16, 6], [23, 3], [6, 0], [20, 2], [14, 1], [22, 9], [1, 14], [2, 12], [15, 0], [9, 11], [6, 15], [24, 12], [12, 9], [2, 4], [13, 11], [15, 10], [6, 7], [10, 3], [4, 8], [21, 10], [18, 6], [11, 2], [21, 6], [2, 13], [27, 14], [12, 5], [26, 4], [10, 15], [8, 4], [14, 15], [9, 6], [6, 4], [16, 5], [6, 5], [12, 12], [0, 15], [5, 15], [11, 0], [28, 4], [5, 4], [4, 15], [21, 8], [10, 11], [0, 4], [9, 5], [3, 11], [25, 5], [1, 13], [11, 8], [8, 5], [12, 1], [12, 10], [21, 3], [3, 4], [11, 12], [8, 14], [8, 7], [19, 6], [8, 3], [25, 13], [23, 4], [26, 0], [11, 1], [27, 8], [20, 4], [11, 13], [13, 2], [25, 10], [0, 10], [27, 15], [10, 6], [24, 5], [7, 0], [2, 15], [15, 5], [27, 7], [26, 15], [8, 2], [24, 10], [7, 8], [22, 4], [7, 10], [25, 8], [13, 7], [23, 9], [12, 15], [26, 6], [11, 10], [26, 7], [29, 15], [29, 4], [10, 8], [22, 7], [24, 4], [10, 1], [20, 9], [10, 0], [13, 0], [28, 7], [22, 5], [1, 4], [29, 6], [23, 8], [22, 8], [3, 13], [28, 15], [10, 10], [3, 14], [29, 12], [27, 6], [18, 13], [20, 5], [0, 11], [12, 3], [11, 14], [17, 4], [19, 9], [9, 4], [26, 2], [18, 9], [7, 1], [0, 5], [8, 6], [2, 11], [14, 5], [6, 10], [15, 4], [20, 6], [5, 14], [8, 10], [19, 5], [24, 7], [20, 8], [27, 5], [10, 7], [12, 13], [3, 10], [7, 11], [12, 8], [7, 6], [4, 10], [17, 15], [0, 12], [29, 13], [28, 5], [21, 4], [8, 1], [28, 11], [24, 8], [29, 3], [1, 15], [7, 4], [26, 8], [10, 14], [11, 3], [9, 14], [27, 9], [13, 13], [12, 2], [23, 15]]
#print("".join(["".join(l) for l in sweeper]).count("1"))
#input()
maxqueue=0
import random
for hit in hits:
    input()
    #hit = input("> ")
    #hitx, hity = hit.strip().split(",")
    #hitx=int(hitx)
    #hity=int(hity)
    hitx = hit[0]
    hity = hit[1]
    if(sweeper[hity][hitx]=="0"):
        print_map()
        #hits.append([hitx, hity])
        colorqueue=[[hitx,hity]]
        while len(colorqueue)>0:
            #print(colorqueue)
            hitx, hity = colorqueue.pop(0)
            print(hex(hitx), hex(hity), hex(hity*30+hitx))
            sweeper[hity][hitx]="X"
            neighbours=[[hitx-1,hity-1],[hitx-1,hity],[hitx-1,hity+1],[hitx,hity-1],[hitx,hity+1],[hitx+1,hity-1],[hitx+1,hity],[hitx+1,hity+1]]
            neighbours=[neighbour for neighbour in neighbours if (neighbour[0]>=0 and neighbour[0]<30 and neighbour[1]>=0 and neighbour[1]<16)]
            print(neighbours)
            print([sweeper[neighbour[1]][neighbour[0]]!="1" for neighbour in neighbours])
            if(all([sweeper[neighbour[1]][neighbour[0]]!="1" for neighbour in neighbours])):
                for neighbour in neighbours:
                    if(sweeper[neighbour[1]][neighbour[0]]=="0"):
                        colorqueue.append(neighbour)
print("".join(["".join(l) for l in sweeper]).count("1"))
print("".join(["".join(l) for l in sweeper]).count("0"))
print(hits)

