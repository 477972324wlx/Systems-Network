#include <bits/stdc++.h>
using namespace std;

typedef int Page;

struct cache{
	int max_count;
	int size;
	
	list<Page> lst;
	unordered_map<Page, list<Page>::iterator > hashmap;
	
	void Put(Page x){
		// already exists , put it to the head;
		if(hashmap.count(x)){
			
			auto it = hashmap[x];
			lst.erase(it);
			hashmap[x] = lst.insert(lst.begin(), x);
			
		} else {
			// not full
			if(size < max_count){
				hashmap[x] = lst.insert(lst.begin(), x);
				size++;
				
			} else {
				//full , replace the least recently used one
				hashmap.erase(lst.back());
				lst.pop_back();
				hashmap[x] = lst.insert(lst.begin(), x);
			}	
		}	
	} 
	void Delete(Page x){
		// element does not exist
		if(!hashmap.count(x)) return;
		
		auto it = hashmap[x];
		lst.erase(it);
		hashmap.erase(x);
		size--;
	}
	void printCache(){
		for (Page x : lst){
			cout << x << ' ';
		}
		cout << endl;
	}
	cache(int MAXCOUNT):max_count(MAXCOUNT){
		lst.clear(); hashmap.clear(); size = 0;
	}
}; 
int main(){
	cache cc(5);
	int x;
	for (int i = 1 ; i <= 11 ; ++i){
		cin >> x;
		cc.Put(x);
		cc.printCache();
	}	
	return 0;	
}