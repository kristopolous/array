A generic array and hash program for shell scripts - implemented in standard c without
external dependencies

# Usage

There are two programs:

 * array
 * hash

If you have modern bash (4.0+) then you actually have pretty decent support for both of these.

If you are using zsh, then you do too.

If you need something really performant for long-running services than may I suggest [redis](http://redis.io) (I've been using it for 3 years, it's great).

If the above case is not you, then let's move forward.

## Exit Codes

When you do an operation, the return value is based upon the operation that happened.  The values are as follows:
<table>
<tr><td>Code</td><td>Meaning</td></tr>
<tr><td>1</td><td>An out of bounds request was made (such as indexing past an array bound)</td></tr>
<tr><td>2</td><td>A void value was emitted.  If you assign a value to index 0 and say 5, on a new array, then [1-4] will be void</td></tr>
<tr><td>3</td><td>The requested array did not exist</td></tr>
<tr><td>4</td><td>The verb used was not identified</td></tr>
<tr><td>5</td><td>The database file specified could not be used or created</td></tr>
<tr><td>6</td><td>The array exists but it is empty and nothing is returned</td></tr>
<tr><td>7</td><td>The array needs to be created but the option to do things explicitly was passed</td></tr>
<tr><td>8</td><td>The array to be deleted does not exist</td></tr>
<tr><td>9</td><td>A value was assigned successfully</td></tr>
<tr><td>10</td><td>An unknown error occured</td></tr>
</table>

Note that you can see these by running

`array|hash --errors`

## array

Array is a tool for either explicitly or implicitly creating arrays that can be easily integrated with other unix tools like awk, sed, and sort.

It uses a database that is currently in a proprietary format, but which will eventually be converted to json.

The basic arguments are

`array [array name] [index] [value]`

Where all parameters are ordered and optional.

### With 0 parameters
Without any parameters, array will list the names of arrays currently stored and there sizes.

### With 1 parameter
With the array name parameter, if the array exists, the exit code is 0 and STDOUT gets an output of the following:

`[index]\t[value]`

If the array does not exist, then STDOUT gets nothing and a non-zero value is returned.

### With 2 parameters
With the array name and index parameter, STDOUT will return the value at the index, followed by a newline.  If there
is no value at the index, the return-value is non-zero, if there is a value, the return value is 0.

### With 3 parameters
With the array name, index, and value parameters, STDOUT returns nothing

### Example
<pre>
$ ./array
$ ./array FirstList 0 value
$ ./array
1       FirstList
$ ./array FirstList 5 value5
$ ./array FirstList
0       value
1       {void}
2       {void}
3       {void}
4       {void}
5       value5
$ ./array FirstList 4
{void}
$ ./array FirstList 5
value5
$ ./array SecondList
$ ./array
6       FirstList
0       SecondList
$ ./array SecondList 123 456
$ ./array
6       FirstList
124     SecondList
$ ./array FirstList delete
$ ./array
124     SecondList
</pre>
# hash

Deprecated in favor of another project of mine, [ticktick](https://github.com/kristopolous/TickTick).
