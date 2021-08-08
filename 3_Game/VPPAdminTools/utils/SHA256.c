/* SHA256 Implementation for DayZ Standalone
 *      By: Kegan 'Lystic' Hollern
 */

// this XOR is global because it is meant to be applied to any datatype
static int VBITWISE_XOR(int x, int y)
{
    int a = (x | y);
    int b = ~x;
    int c = ~y;
    int d = (b | c);
    return (a & d);
}

//vuint definition
typedef int vuint;
class vuint: int
{
    vuint Add(vuint b)
    {
        vuint a = value;
        vuint carry = a & b;
        vuint result = VBITWISE_XOR(a, b);
        while(carry != 0)
        {
            vuint shiftedcarry = carry << 1;
            carry = result & shiftedcarry;
            result = VBITWISE_XOR(result, shiftedcarry);
        }
        return result;
    }
    bool IsGt(vuint b)
    {
        vuint a = value;

        vuint ltb = ~a & b;
        vuint gtb = a & ~b;
        ltb |= ltb.ShiftRight(1);
        ltb |= ltb.ShiftRight(2);
        ltb |= ltb.ShiftRight(4);
        ltb |= ltb.ShiftRight(8);
        ltb |= ltb.ShiftRight(16);
        vuint isGt = gtb & ~ltb;
        return (isGt != 0); //non-zero if A > B
    }
    bool IsLt(vuint b)
    {
        return b.IsGt(value);
    }
    vuint ShiftRight(int amount)
    {
         //because amount is technically (in engine) a signed int, we'll need to check if it's
        if(amount <= 0) 
        {
            //Error(amount.ToString());
            return value;
        }
        vuint new_value = (value >> 1) & 0x7FFFFFFF; //strip the sign bit away (making it 0)
        vuint extra_shifts = amount - 1;
        if(extra_shifts > 0)
            new_value = (new_value >> extra_shifts);

        return new_value;
    }
    //--- unsigned bitwise operations
    vuint ROTLEFT(vbyte b)
    {
        b.Clamp(); //ensure our byte value really is 8 bits

        vuint y = (value << b);
        vuint x = (32 - b);
        vuint z = value.ShiftRight(x);
        return (y | z);

    }
    vuint ROTRIGHT(vbyte b)
    {
        b.Clamp(); //ensure the byte value is clamped
        vuint y = value.ShiftRight(b);
        vuint x = (32 - b);
        vuint z = (value << x); //left shift works on Unsigned Integers
        return (y | z);
    }
    //--- SHA256 based functionality for vUINTs 
    //See: https://csrc.nist.gov/csrc/media/publications/fips/180/4/final/documents/fips180-4-draft-aug2014.pdf
    vuint CH(vuint y, vuint z)
    {
        vuint x = value;
        vuint a = (x & y);
        vuint b = ~x;
        vuint c = (b & z);
        return VBITWISE_XOR(a, c); // a ^ c
    } 
    //i decided to keep this one static because there really isn't a good value in this operation to choose as the base value
    static vuint MAJ(vuint x, vuint y, vuint z)
    {
        vuint a = x & y;
        vuint b = x & z;
        vuint c = y & z;
        return VBITWISE_XOR(a, VBITWISE_XOR(b, c)); //a ^ b ^ c (left-right associativity)
    }
    vuint EP0()
    {
        vuint x = value;
        vuint a = x.ROTRIGHT(2);
        vuint b = x.ROTRIGHT(13);
        vuint c = x.ROTRIGHT(22);
        return VBITWISE_XOR(a, VBITWISE_XOR(b, c)); //a ^ b ^ c
    }
    vuint EP1()
    {
        vuint x = value;
        vuint a = x.ROTRIGHT(6);
        vuint b = x.ROTRIGHT(11);
        vuint c = x.ROTRIGHT(25);
        return VBITWISE_XOR(a, VBITWISE_XOR(b, c)); //a ^ b ^ c
    }
    vuint SIG0()
    {
        vuint x = value;
        vuint a = x.ROTRIGHT(7);
        vuint b = x.ROTRIGHT(18);
        vuint c = x.ShiftRight(3);
        return VBITWISE_XOR(a, VBITWISE_XOR(b, c)); //a ^ b ^ c
    }
    vuint SIG1()
    {
        vuint x = value;
        vuint a = x.ROTRIGHT(17);
        vuint b = x.ROTRIGHT(19);
        vuint c = x.ShiftRight(10);
        return VBITWISE_XOR(a, VBITWISE_XOR(b, c)); //a ^ b ^ c
    }
}

//byte definition
typedef int vbyte;
class vbyte: vuint 
{
    void Clamp()
    {
        value = value & 255; //clamps value 
    }
    static vbyte FromInt(int value)
    {
        vbyte as_byte = value;
        as_byte.Clamp();
        return as_byte;
    }
    string ToHex()
    {
        value.Clamp();
        int ascii = value; //Note: our math isn't working properly on our byte object, so this fixes that
        array<string> characters = {"0","1","2","3","4","5","6","7","8","9","A","B","C","D","E","F"};
        int first = Math.Floor(ascii / 16);
        int last = ascii % 16;
        return (characters.Get(first) + characters.Get(last));
    }
}

//encoding functionality
class VEncoding 
{
    static array<vbyte> GetBytes(string str)
    {
        array<vbyte> result = new array<vbyte>();
        for(int i = 0; i < str.Length();i++)
        {
            string char = str.Get(i);
            vbyte value = VEncoding.GetByte(char);
            result.Insert(value);
        }
        return result;
    }
    
    static vbyte GetByte(string character)
    {
        string element = character.Get(0); //if we send several, just use the first character
        int ascii = element.Hash();
        return vbyte.FromInt(ascii);
    }

    static string FromBytesHex(notnull array<vbyte> data)
    {
        string hexStr = "";
        for(int i = 0; i < data.Count(); i++)
        {
            hexStr += data[i].ToHex();
        }
        return hexStr;
    }
}


//--- SHA256 implementations
class VSHA256_CTX
{
    ref array<vbyte> data;
    vuint datalen;
    ref array<vuint> bitlen;
    ref array<vuint> state;

    void VSHA256_CTX()
    {
        int i;

        datalen = 0;
        data = new array<vbyte>();
        bitlen = new array<vuint>();
        state = new array<vuint>();
        //pre-fill our arrays with 0s
        for(i = 0; i < 64; i++) data.Insert(0);
        for(i = 0; i < 2; i++) bitlen.Insert(0);
        for(i = 0; i < 8; i++) state.Insert(0);
    } 
}

class VSHA256
{
    private ref array<vuint> k = {
        0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
        0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
        0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
        0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
        0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
        0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
        0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
        0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2     
    };

    private ref VSHA256_CTX ctx;
    private ref array<vbyte> hash;
    private bool finished;

    void VSHA256()
    {
        this.ctx = new VSHA256_CTX();
        this.hash = new array<vbyte>();
        for(int i = 0; i < 32; i++) this.hash.Insert(0);
    }

    array<vbyte> Compute(notnull array<vbyte> data, vuint length)
    {
        if(finished)
        {
            Error("SHA256 Object Already Computed! Construct a new object to calculate a new hash! Use the static helper functions!");
            return this.hash;
        }

        Init();
        Update(data, length);
        Final();

        return this.hash;
    }

    private void Init()
    {
        ctx.datalen = 0;
        ctx.bitlen[0] = 0;
        ctx.bitlen[1] = 0;
        ctx.state[0] = 0x6a09e667;
        ctx.state[1] = 0xbb67ae85;
        ctx.state[2] = 0x3c6ef372;
        ctx.state[3] = 0xa54ff53a;
        ctx.state[4] = 0x510e527f;
        ctx.state[5] = 0x9b05688c;
        ctx.state[6] = 0x1f83d9ab;
        ctx.state[7] = 0x5be0cd19;
    }

    private void Update(notnull array<vbyte> data, vuint len)
    {
        for (vuint i = 0; i < len; ++i)
        {
            ctx.data[ctx.datalen] = data[i];
            ctx.datalen++;

            if (ctx.datalen == 64)
            {
                TransformSha(ctx.data);

                array<vuint> results = DBL_INT_ADD(ctx.bitlen[0], ctx.bitlen[1], 512);
                ctx.bitlen[0] = results[0];
                ctx.bitlen[1] = results[1];

                ctx.datalen = 0;
            }
        }
    }

    private void TransformSha(notnull array<vbyte> data)
    {
        vuint a, b, c, d, e, f, g, h, i, j, t1, t2;
        //initialize M to be filled with 64 0s
        array<vuint> m = new array<vuint>();
        for(int m_itr = 0; m_itr < 64; m_itr++) m.Insert(0);

        j = 0;
        for (i = 0; i < 16; ++i)
        {
            vuint l1_a = data[j] << 24;
            vuint l1_b = data[j + 1] << 16;
            vuint l1_c = data[j + 2] << 8;
            vuint l1_d = data[j + 3];
            m[i] = (l1_a | l1_b | l1_c | l1_d);
            j += 4;
        }
        for (; i < 64; ++i)
        {
            vuint l2_a = m[i - 2];
            vuint l2_b = l2_a.SIG1();
            vuint l2_c = m[i - 7];
            vuint l2_d = m[i - 15];
            vuint l2_e = l2_d.SIG0();
            vuint l2_f = m[i - 16];
            m[i] = l2_b + l2_c + l2_e + l2_f;
        }
        a = ctx.state[0];
        b = ctx.state[1];
        c = ctx.state[2];
        d = ctx.state[3];
        e = ctx.state[4];
        f = ctx.state[5];
        g = ctx.state[6];
        h = ctx.state[7];
        for (i = 0; i < 64; ++i)
        {
            t1 = h.Add( e.EP1() ).Add( e.CH(f, g) ).Add( k[i] ).Add( m[i] );//h + EP1(e) + CH(e, f, g) + k[i] + m[i];
            t2 = a.EP0().Add( vuint.MAJ(a, b, c) );//EP0(a) + MAJ(a, b, c);
            h = g;
            g = f;
            f = e;
            e = d.Add(t1);//d + t1;
            d = c;
            c = b;
            b = a;
            a = t1.Add(t2);//t1 + t2;
        }
        ctx.state[0] = ctx.state[0] + a;
        ctx.state[1] = ctx.state[1] + b;
        ctx.state[2] = ctx.state[2] + c;
        ctx.state[3] = ctx.state[3] + d;
        ctx.state[4] = ctx.state[4] + e;
        ctx.state[5] = ctx.state[5] + f;
        ctx.state[6] = ctx.state[6] + g;
        ctx.state[7] = ctx.state[7] + h;
    }

    private void Final()
    {
        vuint i = ctx.datalen;
        if (ctx.datalen < 56)
        {
            ctx.data[i++] = 0x80;

            while (i < 56)
                ctx.data[i++] = 0x00;
        }
        else
        {
            ctx.data[i++] = 0x80;

            while (i < 64)
                ctx.data[i++] = 0x00;

            TransformSha(ctx.data);
        }
        array<vuint> result = DBL_INT_ADD(ctx.bitlen[0], ctx.bitlen[1], ctx.datalen * 8);
        ctx.bitlen[0] = result[0];
        ctx.bitlen[1] = result[1];
        ctx.data[63] = vbyte.FromInt(ctx.bitlen[0]);
        ctx.data[62] = vbyte.FromInt(ctx.bitlen[0].ShiftRight(8));
        ctx.data[61] = vbyte.FromInt(ctx.bitlen[0].ShiftRight(16));
        ctx.data[60] = vbyte.FromInt(ctx.bitlen[0].ShiftRight(24));
        ctx.data[59] = vbyte.FromInt(ctx.bitlen[1]);
        ctx.data[58] = vbyte.FromInt(ctx.bitlen[1].ShiftRight(8));
        ctx.data[57] = vbyte.FromInt(ctx.bitlen[1].ShiftRight(16));
        ctx.data[56] = vbyte.FromInt(ctx.bitlen[1].ShiftRight(24));
        TransformSha(ctx.data);
        for (i = 0; i < 4; ++i)
        {
            int shift = 24 - (i * 8);
            hash[i] = vbyte.FromInt(ctx.state[0].ShiftRight(shift));
            hash[i + 4] = vbyte.FromInt(ctx.state[1].ShiftRight(shift));
            hash[i + 8] = vbyte.FromInt(ctx.state[2].ShiftRight(shift));
            hash[i + 12] = vbyte.FromInt(ctx.state[3].ShiftRight(shift));
            hash[i + 16] = vbyte.FromInt(ctx.state[4].ShiftRight(shift));
            hash[i + 20] = vbyte.FromInt(ctx.state[5].ShiftRight(shift));
            hash[i + 24] = vbyte.FromInt(ctx.state[6].ShiftRight(shift));
            hash[i + 28] = vbyte.FromInt(ctx.state[7].ShiftRight(shift));
        }
    }

    //This function treats A and B as one 64bit unsigned integer and adds C to it
    private array<vuint> DBL_INT_ADD(vuint a, vuint b, vuint c)
    {
        vuint max_int = 0xffffffff;
        vuint test = max_int - c;
        if(a.IsGt(test)) //if a > test (unsigned comparison)
        {
            ++b;
        }

        a = a.Add(c); // A + C using unsigned math

        array<vuint> results = new array<vuint>();
        results.Insert(a);
        results.Insert(b);
        return results;
    }
    
    static array<vbyte> ComputeArray(notnull array<vbyte> data, vuint length)
    {
        VSHA256 crypto = new VSHA256();
        return crypto.Compute(data, length);
    }

    static string ComputeString(string data)
    {
        return VEncoding.FromBytesHex(VSHA256.ComputeArray(VEncoding.GetBytes(data), data.Length()));
    }
}