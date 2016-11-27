import sys
import xml.sax
from xml.dom import minidom, Node
from xml.etree import ElementTree
from collections import defaultdict



class XMLTVParserException(Exception):
    pass



class Parser(object):

    def join(self, channels, programme):

        for channel in channels:
            channels[channel]['prog'] = sorted(programme[channel], key = lambda x: x['start'])

        return channels



class DOMParserLight(Parser):

    def parse(self, fileobj):
        channels = {}
        programme = defaultdict(list)

        root = ElementTree.parse(fileobj).getroot()

        for child in root:
            try:
                if child.tag == 'channel':
                    channels[child.get('id')] = {
                        'name' : child.find('display-name').text,
                        'prog' : None
                    }

                elif child.tag == 'programme':
                    title    = child.find('title').text
                    category = child.findtext('category', '')
                    desc     = child.findtext('desc', '')

                    programme[child.get('channel')].append({
                        'start'     : child.get('start'),
                        'stop'      : child.get('stop'),
                        'title'     : title,
                        'desc'      : desc,
                        'category'  : category
                    })

            except:
                print("parsing error occured", file = sys.stderr)

        return self.join(channels, programme)



class DOMParser(Parser):

    def parse(self, fileobj):
        channels = {}
        programme = defaultdict(list)

        root = minidom.parse(fileobj).documentElement

        for child in root.childNodes:
            try:
                if child.nodeType == Node.ELEMENT_NODE and child.tagName == 'channel':
                    channels[child.getAttribute('id')] = {
                        'name' : child.getElementsByTagName('display-name')[0].firstChild.data,
                        'prog' : None
                    }

                elif child.nodeType == Node.ELEMENT_NODE and child.tagName == 'programme':
                    title = child.getElementsByTagName('title')[0].firstChild.data

                    try:
                        desc = child.getElementsByTagName('desc')[0].firstChild.data
                    except:
                        desc = ""

                    try:
                        category = child.getElementsByTagName('category')[0].firstChild.data
                    except:
                        category = ""

                    programme[child.getAttribute('channel')].append({
                        'start'     : child.getAttribute('start'),
                        'stop'      : child.getAttribute('stop'),
                        'title'     : title,
                        'desc'      : desc,
                        'category'  : category
                    })

            except:
                print("parsing error occured", file = sys.stderr)

        return self.join(channels, programme)



class SAXParser(Parser):

    class XMLTVHandler(xml.sax.handler.ContentHandler):

        def __init__(self, channels, programme):
            self.channels = channels
            self.programme = programme

            self.stack = []
            self.last_channel = None
            self.last_program = None

        def startElement(self, name, attrs):
            self.stack.append(name)

            if name == 'channel':
                channel = attrs['id']
                self.last_channel = channel
                self.channels[channel] = {
                    'name' : "",
                    'prog' : None
                }

            elif name == 'programme':
                program = {
                    'start'     : attrs['start'],
                    'stop'      : attrs['stop'],
                    'title'     : "",
                    'desc'      : "",
                    'category'  : ""
                }

                self.last_program = program
                self.programme[attrs['channel']].append(program) 

        def endElement(self, name):
            if (self.stack[-1] != name):
                raise XMLTVParserException("wrong file format")
            
            self.stack.pop()

        def characters(self, content):
            if self.stack[-1] == 'display-name':
                self.channels[self.last_channel]['name'] += content
            
            if self.stack[-1] == 'title':
                self.last_program['title'] += content

            if self.stack[-1] == 'desc':
                self.last_program['desc'] += content

            if self.stack[-1] == 'category':
                self.last_program['category'] += content            


    def parse(self, fileobj):
        channels = defaultdict(str)
        programme = defaultdict(list)

        sax = xml.sax.make_parser()
        sax.setContentHandler(SAXParser.XMLTVHandler(channels, programme))
        sax.parse(fileobj)

        return self.join(channels, programme)
            


_impls = {
    'sax'   : SAXParser,
    'dom'   : DOMParser,
    'light' : DOMParserLight
}

def getImplementation(parser_name):
    if not parser_name in _impls:
        raise XMLTVParserException("wrong parser name")

    return _impls[parser_name]()



if __name__ == '__main__':    

    # ---------- example ---------- #    
    from datetime import datetime
    import gzip

    parser = getImplementation('sax')
    programme = parser.parse(gzip.open('ttv.xmltv.xml.gz'))

    for item in programme['ttv401']['prog']:
        print("%(from)s - %(to)s\t%(title)s" % {
            'from'  : datetime.strptime(item['start'], "%Y%m%d%H%M%S %z").strftime('%d %b %H:%M'),
            'to'    : datetime.strptime(item['stop'], "%Y%m%d%H%M%S %z").strftime('%H:%M'),
            'title' : item['title']
        })       