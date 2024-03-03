#include <iostream>
#include <cstring>
#include <vector>
#include <bitset>
#include <algorithm>
#include <stdint.h>
#include <fstream>
#include <filesystem>

/**
 * @brief *.hfmtree : file coded with huffman tree
 * [sizeof(std::size_t) bytes]: l1, typed std::size_t, the size of space recording HFMTree::tree
 * [l1 bytes]: HFMTree::tree, recorded in form of [0b10000000][left][root][right][0b10000001]
 * [sizeof(std::size_t) bytes]: l2, typed std::size_t, the size of following codes in bits !!!bits!!!
 * [l2 bits]: coded text !!!bits!!!
 */

/**
 * @brief
 * Huffman tree, containing a huffman tree (stored in binary tree) and code (for each character)
 */
class HFMTree
{
public:
    class Counter;
    friend class HFMString;

private:
    class Node;
    using Tree = Node *;
    class Weighted_Node;
    using Heap = std::vector<Weighted_Node>;
    class Code;

    Tree tree;
    std::vector<Code> code;

private:
    /** @brief helper class for HFMTree::tree, storing a huffman tree in a binary tree */
    class Node
    {
    public:
        /** @brief left child */
        Node *left;
        /** @brief right child */
        Node *right;
        /** @brief value, EOF for non-leaf Nodes, corresponding character for leaf Nodes */
        char value;

        /** @brief default constructor */
        Node() : left(nullptr), right(nullptr), value(EOF) {}
        /** @brief Copy Constructor, copying the whole tree */
        Node(const Node &other) = delete;
        /** @brief Move Constructor */
        Node(Node &&other) : left(other.left), right(other.right), value(other.value)
        {
            other.left = nullptr;
            other.right = nullptr;
        }

        /**
         * @brief Construct a new Node object (leaf Node) with a char
         * @param v const char &
         */
        Node(const char &v) : left(nullptr), right(nullptr), value(v) {}
        /**
         * @brief Construct a new Node object with 2 sub-trees
         * @param l left sub-tree
         * @param r right sub-tree
         * @param v value, defaults for EOF
         */
        Node(Tree l, Tree r, const char &v = EOF) : left(l), right(r), value(v) {}

        /** @brief Destructor. Destroy the Node object only, use HFMTree::destroy_tree(Tree t) instead to destroy the whole tree */
        virtual ~Node() = default;

        // Node &operator=(const Node &other) = default;
        Node &operator=(Node &&other) = default;
    };
    /**
     * @brief helper function for std::vector<uint8_t> sequence()
     * @param v storing result, typed std::vector<uint8_t>&
     * @param t Tree
     */
    static void _sequence(std::vector<uint8_t> &v, Tree t)
    {
        v.emplace_back(0b10000000);
        if (t->left)
        {
            _sequence(v, t->left);
            v.emplace_back(uint8_t(t->value));
            _sequence(v, t->right);
        }
        else
            v.emplace_back(t->value);
        v.emplace_back(0b10000001);
        return;
    }
    /**
     * @brief sequence a HFMTree tree
     * @return std::vector<uint8_t> sequenced HFMTree object
     */
    std::vector<uint8_t> sequence() const
    {
        std::vector<uint8_t> v(0);
        _sequence(v, tree);
        return v;
    }

    /** @brief helper class for HFMTree::tree, storing a huffman tree in a binary tree with weight*/
    class Weighted_Node : public Node
    {
    public:
        /** @brief weight of the whole tree */
        std::size_t weight;

        /** @brief default constructor */
        Weighted_Node() : Node(), weight(0) {}

        /** @brief Copy Constructor, copying the whole tree */
        Weighted_Node(const Weighted_Node &other) = delete;
        /** @brief Move Constructor */
        Weighted_Node(Weighted_Node &&other) : weight(other.weight)
        {
            value = other.value;
            left = other.left;
            right = other.right;
            other.left = nullptr;
            other.right = nullptr;
        }

        /**
         * @brief Construct a new Node object (leaf Node) with a char
         * @param v char
         */
        Weighted_Node(const char &v, const std::size_t &w = 0) : Node(v), weight(w) {}
        /**
         * @brief Construct a new Weighted_Node object with 2 sub-trees
         * @param l left sub-tree
         * @param r right sub-tree
         * @param w weight
         */
        Weighted_Node(Tree l, Tree r, const std::size_t w) : Node(l, r), weight(w) {}

        /** @brief Destructor. Destroy the Node object only, use HFMTree::destroy_tree(Tree t) instead to destroy the whole tree */
        virtual ~Weighted_Node() = default;

        // Weighted_Node &operator=(const Weighted_Node &other) = default;
        Weighted_Node &operator=(Weighted_Node &&other) = default;

        /** @brief operator== comparing 2 Nodes by weight */
        friend inline bool operator>=(const Weighted_Node &a, const Weighted_Node &b) { return a.weight >= b.weight; }
        constexpr static auto greater_equal = std::greater_equal<const Weighted_Node &>();
    };

    /** @brief helper class for HFMTree::code */
    class Code : private std::vector<bool>
    {
    public:
        Code() = default;
        /**
         * @brief Construct a new Code objecty
         * @param v const reference to a std::vector<bool> object
         * @param l length
         */
        Code(const std::vector<bool> &v, const uint8_t &l) : std::vector<bool>(v) { resize(l); }
        Code(const Code &) = default;
        Code(Code &&) = default;
        Code &operator=(const Code &) = default;
        Code &operator=(Code &&) = default;
        virtual ~Code() = default;

        inline std::size_t size() const { return std::vector<bool>::size(); }
        inline auto operator[](std::size_t index) { return std::vector<bool>::operator[](index); }
        inline auto operator[](std::size_t index) const { return std::vector<bool>::operator[](index); }
    };

    static Tree copy_tree(Tree tree)
    {
        if (tree == nullptr)
            return nullptr;
        return new Node(copy_tree(tree->left), copy_tree(tree->right), tree->value);
    }

    /**
     * @brief destroy a tree
     * @param tree
     */
    static void destroy_tree(Tree tree)
    {
        if (tree)
        {
            if (tree->left)
            {
                destroy_tree(tree->left);
                destroy_tree(tree->right);
            }
            delete tree;
        }
        return;
    }

    /**
     * @brief building a huffman tree using a HFMTree::Heap reference
     * @param heap counting characters, typed HFMTree::Heap&
     * @return Tree
     */
    static Tree build_tree(Heap &heap)
    {
        std::make_heap(heap.begin(),
                       heap.end(),
                       Weighted_Node::greater_equal);
        while (!heap.front().weight)
        {
            std::pop_heap(heap.begin(), heap.end(), Weighted_Node::greater_equal);
            heap.pop_back();
        }
        if (heap.size() == 0)
            throw std::runtime_error("empty text passed to class HFMTree.");
        if (heap.size() == 1)
            throw std::runtime_error("single-character-composed text passed to class HFMTree.");
        Weighted_Node left, right;
        std::size_t weight;
        while (heap.size() > 1)
        {
            std::pop_heap(heap.begin(), heap.end(), Weighted_Node::greater_equal);
            left = std::move(heap.back());
            heap.pop_back();
            std::pop_heap(heap.begin(), heap.end(), Weighted_Node::greater_equal);
            right = std::move(heap.back());
            weight = left.weight + right.weight;
            heap.back() = Weighted_Node(new Node(std::move(left)), new Node(std::move(right)), weight);
            std::push_heap(heap.begin(), heap.end(), Weighted_Node::greater_equal);
        }
        return new Node(std::move(heap[0]));
    }
    /**
     * @brief building a huffman tree using a std::string
     * @param s source text
     * @return Tree
     */
    static Tree build_tree(const std::string &s)
    {
        Heap heap(128);
        for (std::size_t i = 0; i < 128; i++)
            heap[i].value = char(i);
        for (const auto &i : s)
            heap[i].weight++;
        return build_tree(heap);
    }
    /**
     * @brief building a huffman tree using a Counter object
     * @param counter counting characters, typed HFMTree::Counter
     * @return Tree
     */
    static Tree build_tree(const Counter &counter)
    {
        Heap heap(128);
        for (std::size_t i = 0; i < counter.size(); i++)
        {
            heap[i].value = char(i);
            heap[i].weight = counter[i];
        }
        return build_tree(heap);
    }
    /**
     * @brief build a huffman tree using a sequenced tree [begin, end)
     * @tparam T iterator type
     * @param begin begin of the sequence
     * @param end end of the sequence
     * @return Tree
     */
    template <typename RandomAccessIterator>
    static Tree build_tree(const RandomAccessIterator &begin, const RandomAccessIterator &end)
    {
        if (begin + 1 == end)
            return nullptr;
        RandomAccessIterator p = begin + 1;
        if (*p == 0b10000000)
        {
            uint8_t count = 1;
            p++;
            while (count)
            {
                if (*p == 0b10000000)
                    count++;
                else if (*p == 0b10000001)
                    count--;
                p++;
            }
            return new Node(build_tree(begin + 1, p), build_tree(p + 1, end - 1), *p);
        }
        else
            return new Node(nullptr, nullptr, *p);
    }

    /**
     * @brief helper function for static void generate_code(Tree tree, Code code[])
     * @param tree HFMTree::tree
     * @param code HFMTree::code
     * @param path recording current path, typed std::vector<bool>
     * @param depth recorcing current depth (also path length), typed uint8_t
     */
    static void _generate_code(Tree tree, std::vector<Code> &code, std::vector<bool> &path, const uint8_t &depth)
    {
        if (!(tree->left || tree->right))
            code[std::size_t(tree->value)] = Code(path, depth);
        else
        {
            path[depth] = 1;
            _generate_code(tree->right, code, path, depth + 1);
            path[depth] = 0;
            _generate_code(tree->left, code, path, depth + 1);
        }
        return;
    }
    /**
     * @brief helper function for HFMTree(const std::string &s), generating HFMTree::code
     *
     * @param tree HFMTree::tree
     * @param code HFMTree::code
     */
    static std::vector<Code> generate_code(Tree tree)
    {
        std::vector<Code> code(128);
        std::vector<bool> path(128);
        _generate_code(tree, code, path, 0);
        return code;
    }

    /**
     * @brief get a huffman tree object from std::fstream, will be used in class HFMString
     * @param i std::fstream, required to be opened in binary mode
     * @param h HFMTree object
     * @return std::fstream&
     */
    void get_from_fstream(std::fstream &i)
    {
        std::size_t l1;
        i.read((char *)(&l1), sizeof(std::size_t));
        uint8_t *p = new uint8_t[l1];
        i.read((char *)(p), l1);
        *this = HFMTree(p, p + l1);
        delete[] p;
        return;
    }

public:
    /** @brief counting characters, Counter.size() should be 128*/
    class Counter : private std::vector<std::size_t>
    {
    public:
        Counter() : std::vector<std::size_t>(128) {}
        Counter(const Counter &) = default;
        Counter(Counter &&) = default;
        virtual ~Counter() = default;

        inline std::size_t &operator[](std::size_t index) noexcept { return std::vector<std::size_t>::operator[](index); }
        inline const std::size_t &operator[](std::size_t index) const noexcept { return std::vector<std::size_t>::operator[](index); }
        inline std::size_t size() const noexcept { return std::vector<std::size_t>::size(); }
    };

    HFMTree() : tree(nullptr), code(128) {}
    HFMTree(const HFMTree &other) : tree(copy_tree(other.tree)), code(other.code) {}
    HFMTree(HFMTree &&other) : tree(other.tree), code(std::move(other.code)) { other.tree = nullptr; }
    HFMTree(const std::string &s) : tree(build_tree(s)), code(generate_code(tree)) {}
    HFMTree(const char *s) : tree(build_tree(std::string(s))), code(generate_code(tree)) {}
    HFMTree(const Counter &c) : tree(build_tree(c)), code(generate_code(tree)) {}
    template <typename RandomAccessIterator>
    HFMTree(const RandomAccessIterator &begin, const RandomAccessIterator &end)
        : tree(build_tree(begin, end)), code(generate_code(tree)) {}
    virtual ~HFMTree() { destroy_tree(tree); }
    HFMTree &operator=(const HFMTree &other)
    {
        tree = copy_tree(other.tree);
        code = other.code;
        return *this;
    }
    HFMTree &operator=(HFMTree &&other)
    {
        tree = other.tree;
        code = std::move(other.code);
        other.tree = nullptr;
        return *this;
    }

    /**
     * @brief encode a string with the HFMTree object
     * @param string string to be encoded, typed const string&
     * @return std::vector<uint8_t> encoded string, first [sizeof(std::size_t) bytes] for length of following code
     */
    std::vector<uint8_t> encode(const std::string &string) const
    {
        std::vector<uint8_t> result(sizeof(std::size_t));
        uint8_t cache(0b00000000), p(0b10000000);
        for (const auto &i : string)
        {
            const auto &c = code[std::size_t(i)];
            for (std::size_t j = 0; j < c.size(); j++)
            {
                if (c[j])
                    cache |= p;
                p >>= 1;
                if (!p)
                {
                    result.emplace_back(cache);
                    cache = 0b00000000;
                    p = 0b10000000;
                }
            }
        }
        uint8_t extra = 0;
        if (p != 0b10000000)
        {
            result.emplace_back(cache);
            do
                extra++;
            while (p >>= 1);
        }
        *((std::size_t *)(&(result[0]))) = ((result.size() - sizeof(std::size_t)) << 3) - extra;
        return result;
    }
    /**
     * @brief decode HFMString::code with the HFMTree object
     * @tparam RandomAccessIterator
     * @param begin iterator pointing to the beginning of the code
     * @param end iterator pointing to the end of the code
     * @param l2 length of the code in !!!bits!!!
     * @return std::string
     */
    template <typename RandomAccessIterator>
    std::string decode(const RandomAccessIterator &begin, const RandomAccessIterator &end, const std::size_t &l2) const
    {
        std::vector<char> v;
        std::size_t count = 0;
        uint8_t p = 0b10000000;
        Tree t = tree;
        for (auto _i = begin; _i != end; _i++)
        {
            const auto &i = *_i;
            for (p = 0b10000000; p; p >>= 1)
            {
                t = (p & i) ? t->right : t->left;
                count++;
                if (t->value != EOF)
                {
                    v.push_back(t->value);
                    t = tree;
                    if (count == l2)
                        break;
                }
            }
        }
        return std::string(v.begin(), v.end());
    }
    /**
     * @brief decode HFMString::code with the HFMTree object
     * @param code HFMString::code to be decoded !!!WITHOUT!!! the part storing l2, typed std::vector<uint8_t>,
     * @return std::string
     */
    inline std::string decode(const std::vector<uint8_t> &code, const std::size_t &l2) const { return decode(code.begin(), code.end(), l2); }
    /**
     * @brief decode HFMString::code with the HFMTree object
     * @param code HFMString::code to be decoded, !!!WITH!!! the part storing l2 typed std::vector<uint8_t>
     * @return std::string
     */
    inline std::string decode(const std::vector<uint8_t> &code) const
    {
        std::size_t l2 = *((std::size_t *)(&code[0]));
        return decode(code.begin() + sizeof(std::size_t), code.end(), l2);
    }

    /**
     * @brief operator<< reloaded for std::fstream
     * @param o std::fstream, required to be opened in binary mode
     * @param h HFMTree object
     * @return std::fstream&
     */
    friend std::fstream &operator<<(std::fstream &o, const HFMTree &h)
    {
        auto s = h.sequence();
        std::size_t l1 = s.size();
        o.write((const char *)(&l1), sizeof(std::size_t));
        o.write((const char *)(&(s[0])), s.size());
        return o;
    }

    /** @brief print tree(sequenced)*/
    void print_tree() const
    {
        auto s = sequence();
        for (const auto &i : s)
        {
            if (i == 0b10000000)
                std::cout << '(';
            else if (i == 0b10000001)
                std::cout << ')';
            else
                std::cout << uint16_t(i);
        }
        std::cout << std::endl;
        return;
    }
};

class HFMString
{
private:
    std::string string;
    HFMTree hfmtree;
    std::vector<uint8_t> code;

public:
    HFMString() = default;
    HFMString(const HFMString &) = default;
    HFMString(HFMString &&) = default;
    HFMString(const std::string &s) : string(s), hfmtree(string), code(hfmtree.encode(string)) {}
    HFMString(std::string &&s) : string(s), hfmtree(string), code(hfmtree.encode(string)) {}
    HFMString(const char *s) : string(s), hfmtree(string), code(hfmtree.encode(string)) {}
    HFMString(const HFMTree &h, const std::string &s) : string(s), hfmtree(h), code(hfmtree.encode(string)) {}
    HFMString(HFMTree &&h, const std::string &s) : string(s), hfmtree(h), code(hfmtree.encode(string)) {}
    HFMString(const HFMTree &h, std::string &&s) : string(s), hfmtree(h), code(hfmtree.encode(string)) {}
    HFMString(HFMTree &&h, std::string &&s) : string(s), hfmtree(h), code(hfmtree.encode(string)) {}
    HFMString(const std::filesystem::path &path)
    {
        const std::string mode(path.extension().string());
        if (mode == ".hfmtree")
        {
            std::fstream i(path, std::ios::in | std::ios::binary);
            // initialize hfmtree
            hfmtree.get_from_fstream(i);
            // initialize code
            std::size_t l2;
            i.read((char *)(&l2), sizeof(std::size_t));
            std::size_t byte_count = (l2 + 0b111) >> 3;
            code = std::vector<uint8_t>(byte_count);
            i.read((char *)(&(code[0])), byte_count);
            // initialize string
            string = hfmtree.decode(code, l2);
            i.close();
        }
        else if (mode == ".txt")
        {
            std::fstream i(path, std::ios::in);
            string = std::string(std::istreambuf_iterator<char>(i), std::istreambuf_iterator<char>());
            hfmtree = HFMTree(string);
            code = hfmtree.encode(string);
        }
        else
            throw std::invalid_argument("invalid file used to construct a HFMString object");
    }
    virtual ~HFMString() = default;
    operator std::string() const { return string; }

    friend std::fstream &operator<<(std::fstream &o, const HFMString &h)
    {
        o << h.hfmtree;
        o.write((const char *)(&(h.code[0])), h.code.size());
        return o;
    }
    /**
     * @brief write a HFMString object into a file
     * @param p path of targeting file, defaults to "a.hfmtree"
     */
    void write(const std::filesystem::path &p = std::filesystem::path())
    {
        std::fstream o((p == std::filesystem::path()) ? std::filesystem::path("a.hfmtree") : p,
                       std::ios::out | std::ios::binary);
        o << *this;
        o.close();
        return;
    }
};

int main(const int argc, const char **argv)
{
    // // test #1
    // uint64_t n;
    // HFMTree::Counter counter;
    // std::cin >> n;
    // std::cin.get();
    // char chr;
    // while(n--)
    // {
    //     std::cin.get(chr);
    //     std::cin >> counter[chr];
    //     std::cin.get();
    // }
    // char s[1024];
    // std::cin.getline(s, 1024);
    // auto ht = HFMTree(counter);
    // ht.print_tree();
    // auto encode = ht.encode(s);
    // auto decode = ht.decode(encode);
    // std::cout << decode << std::endl;

    // test #2
    using it = std::istreambuf_iterator<char>;
    HFMString A(std::filesystem::path("2_origin.txt"));
    A.write("2_encoded.hfmtree");
    HFMString B(std::filesystem::path("2_encoded.hfmtree"));
    std::fstream o("2_decoded.txt", std::ios::out);
    o << std::string(B) << std::endl;
    o.close();
    std::fstream i1("2_origin.txt", std::ios::in), i2("2_decoded.txt", std::ios::in);
    std::string s1{it(i1), it()}, s2{it(i2), it()};
    i1.close();
    i2.close();
    bool error = false;
    for (std::size_t i = 0; i < s1.length(); i++)
    {
        if (s1[i] != s2[i])
        {
            error = true;
            std::cout << "(@" << i << ")" << std::endl;
            break;
        }
    }
    std::cout << (error ? "Error" : "No Error") << std::endl;

    return 0;
}