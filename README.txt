JSON-XML

Converts JSON to XML, reading from standard input and writing to standard
output:

  json-xml <test.json
   or
  cat test.json | json.xml

Element names are assigned like so:

  1. The root object:    <root>
  2. For array elements: <item>
  3. For keyed values:   the key name, where
      a. All non-alphanumeric characters are replaced by an underscore
      b. An underscore is prefixed if the name starts with a number
      c. Subsequent underscores are merged

A "type" attributed is assigned one of:

  null string number boolean object array

Example:

  {}       <root type="object"></root>
  1        <root type="number">1</root>
  1.0      <root type="number">1.0</root>
  true     <root type="boolean">true</root>
  null     <root type="null" />

  [1,2]    <root type="array">
             <item type="number">1</item>
             <item type="number">2</item>
           </root>

  {"a":1, "b": 2}
           <root type="object">
             <a type="number">1</a>
             <b type="number">2</b>
           </root>

  {"1 2 (3)": null}
           <root type="object">
             <_1_2_3 type="null" />
           </root>

See also:
  LICENSE.txt   copyright information.
  BUILDING.txt  how to build this program.

Written by Sijmen J. Mulder <ik@sjmulder.nl>, 2017.

