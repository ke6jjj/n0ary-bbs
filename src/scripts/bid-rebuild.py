import sys
import collections

MessageFile = collections.namedtuple('MessageFile', 'r_header body bbs_header')

def message_from_stream(st):
    r_header = []
    body = []
    bbs_header = {}
    for line in st:
        if line.rstrip() == '':
            break
        r_header.append(line)
    for line in st:
        if line.rstrip() == '/EX':
            break
        body.append(line)
    for line in st:
        if line.rstrip() == '':
            continue
        key, value_r = line.rstrip().split(':', 1)
        value = value_r.lstrip()
        bbs_header[key.lower()] = value

    return MessageFile(r_header, body, bbs_header)


def print_bid(message_file):
    header = message_file.bbs_header

    bid = header.get('x-bid')
    if bid is None:
        return
    create_time_t = int(header.get('x-create'))
    print '+%s %d' % (bid.upper(), create_time_t)

def main():
    import sys

    for msg_f in sys.argv[1:]:
        with open(msg_f, 'rt') as f:
            m = message_from_stream(f)
        print_bid(m)

main()
